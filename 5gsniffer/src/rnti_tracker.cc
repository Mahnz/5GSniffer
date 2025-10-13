#include "rnti_tracker.hpp"
#include <cstdio>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <algorithm>


// —————————————————————————— Helpers ——————————————————————————
static inline std::string json_escape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c)
    {
    case '\"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[7];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        out += buf;
      } else {
        out += c;
      }
    }
  }
  return out;
}

// ——————————————————————————————————————————————————————————————

JsonPerRntiSink::JsonPerRntiSink(const std::string &path) : path_(path + ".json") {}

void JsonPerRntiSink::on_event(const std::string &/*event_type*/, const RntiRecord &r) {
  records_[r.rnti] = r; // Updated record of events per RNTI
  write_all();
}

void JsonPerRntiSink::flush() {
  write_all();
}

double JsonPerRntiSink::logical_now() const {
  double now_s = 0.0;
  for (const auto& kv : records_) {
    now_s = std::max(now_s, kv.second.last_seen);
  }
  return now_s;
}

void JsonPerRntiSink::write_all() {
  const double now_s = logical_now();

  const std::string tmp = path_ + ".tmp";
  std::ofstream f(tmp, std::ios::trunc);

  f << "{\"generated_at_s\":" << std::fixed << std::setprecision(6) << now_s
    << ",\"ttl_seconds\":" << ttl_seconds_ << ",\"rntis\":[";

  bool first_rec = true;
  for (const auto &kv : records_) {
    const auto &r = kv.second;
    if (!first_rec) {
      f << ",\n";
    }
    first_rec = false;
    
    f << "{"
      << "\"rnti\":" << r.rnti << ",";
    
    const bool active = (ttl_seconds_ > 0.0) ? ((now_s - r.last_seen) <= ttl_seconds_) : true;
    f << "\"active\":" << (active ? "true":"false") << ",";
    
    f << "\"revivals\":" << r.revivals << ","
      << "\"first_seen\":" << std::fixed << std::setprecision(6) << r.first_seen << ","
      << "\"last_seen\":" << std::fixed << std::setprecision(6) << r.last_seen << ","
      << "\"first_sample\":" << r.first_sample << ","
      << "\"last_sample\":" << r.last_sample << ","
      << "\"seen_count\":" << r.seen_count << ","
      << "\"events\":[";
    
    // Block of events
    bool first_ev = true;
    for (const auto &ev : r.events) {
      if (!first_ev) f << ",";
      first_ev = false;
      f << "{"
        << "\"t\":" << std::fixed << std::setprecision(6) << ev.t_seconds << ","
        << "\"cell_id\":" << ev.cell_id << ","
        << "\"scrambling_id\":" << ev.scrambling_id << ","
        << "\"aggregation_level\":" << (int)ev.aggregation_level << ","
        << "\"correlation\":" << ev.correlation << ","
        << "\"sample\":" << ev.sample_index << ","
        << "\"slot\":" << (int)ev.slot << ","
        << "\"ofdm_symbol\":" << (int)ev.ofdm_symbol << ","
        << "\"candidate_idx\":" << (int)ev.candidate_idx
        << "}";
    }
    f << "]}";
  }
  f << "]}";
  f.close();

  std::error_code ec;
  std::filesystem::rename(tmp, path_, ec);
  if (ec) {
    std::filesystem::copy_file(tmp, path_, std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::remove(tmp, ec);
  }
}


// ——————————————————— Tracker Initialization ———————————————————
std::unique_ptr<IRntiSink> RntiTracker::make_sink(const std::string &format, const std::string &path) {
  return std::make_unique<JsonPerRntiSink>(path);
}

RntiTracker &RntiTracker::instance() {
  static RntiTracker inst;
  return inst;
}

void RntiTracker::configure(const std::string &output_path,
                            const std::string &format,
                            double ttl_seconds) {
  std::lock_guard<std::mutex> lk(mu_);
  ttl_seconds_ = ttl_seconds;
  sink_ = make_sink(format, output_path);

  if (auto *s = dynamic_cast<JsonPerRntiSink*>(sink_.get())) {
    s->set_config(format, ttl_seconds_);
  }
}

void RntiTracker::observe(const RntiEvent &ev) {
  std::lock_guard<std::mutex> lk(mu_);

  auto it = table_.find(ev.rnti);
  const bool is_new = (it == table_.end());
  if (is_new) {
    RntiRecord r{};
    r.rnti = ev.rnti;
    r.first_seen = ev.t_seconds;
    r.last_seen = ev.t_seconds;
    r.first_sample = ev.sample_index;
    r.last_sample = ev.sample_index;
    r.seen_count = 1;
    r.revivals = 0;

    // First event
    r.events.push_back(ev);
    it = table_.emplace(ev.rnti, std::move(r)).first;

    // O(1): nuovo RNTI inserito -> incrementa contatore attivi
    active_count_++;

  } else {
    RntiRecord &r = it->second;

    const double gap = ev.t_seconds - r.last_seen;
    if (gap > ttl_seconds_) {
      r.revivals++;
      // r.last_inactive_gap_s = gap;
    }
    
    r.last_seen = ev.t_seconds;
    r.last_sample = ev.sample_index;
    r.seen_count++;

    // Next event
    r.events.push_back(ev);
  }

  if (sink_) {
    sink_->on_event(is_new ? "new" : "update", it->second);
  }
}

void RntiTracker::expire_older_than(double cutoff_s) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto it = table_.begin(); it != table_.end();) {
    if (it->second.last_seen < cutoff_s) {
      // O(1): RNTI scaduto -> decrementa contatore attivi
      if (active_count_ > 0) {
        --active_count_;
      }
      it = table_.erase(it);
    } else {
      ++it;
    }
  }
}

// TODO - Evaluate which alternative is better
// size_t RntiTracker::active_count(double now_s) const {
//   std::lock_guard<std::mutex> lk(mu_);
//   size_t n = 0;
//   for (const auto &kv : table_) {
//     if (now_s - kv.second.last_seen <= ttl_seconds_)
//       n++;
//   }
//   return n;
// }
size_t RntiTracker::active_count(double /*now_s*/) const {
  std::lock_guard<std::mutex> lk(mu_);  // thread-safe, to keep data consistency
  return active_count_;
}

std::optional<RntiRecord> RntiTracker::get(uint16_t rnti) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = table_.find(rnti);
  if (it == table_.end())
    return std::nullopt;
  return it->second;
}

void RntiTracker::flush() {
  std::lock_guard<std::mutex> lk(mu_);
  if (sink_)
    sink_->flush();
}
