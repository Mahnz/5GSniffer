#include "rnti_tracker.hpp"
#include <cstdio>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>


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

// ———————————————————— JSON events per line ————————————————————
JsonlRntiSink::JsonlRntiSink(const std::string &path) : path_(path) {}

void JsonlRntiSink::on_event(const std::string &event_type, const RntiRecord &r) {
  std::ofstream f(path_, std::ios::app);
  f << "{"
    << "\"event\":\"" << json_escape(event_type) << "\","
    << "\"rnti\":" << r.rnti << ","
    << "\"first_seen\":" << std::fixed << std::setprecision(6) << r.first_seen << ","
    << "\"last_seen\":" << std::fixed << std::setprecision(6) << r.last_seen << ","
    << "\"first_sample\":" << r.first_sample << ","
    << "\"last_sample\":" << r.last_sample << ","
    << "\"seen_count\":" << r.seen_count << ","
    << "\"cell_id\":" << r.cell_id << ","
    << "\"scrambling_id\":" << r.scrambling_id << ","
    << "\"coreset_id\":" << (int)r.coreset_id << ","
    << "\"aggregation_level\":" << (int)r.aggregation_level << ","
    << "\"candidate_idx\":" << (int)r.candidate_idx << ","
    << "\"slot\":" << (int)r.slot << ","
    << "\"ofdm_symbol\":" << (int)r.ofdm_symbol << ","
    << "\"num_symbols_per_slot\":" << (int)r.num_symbols_per_slot << ","
    << "}\n";
}

// ——————————————————————— JSON per RNTI ———————————————————————
JsonPerRntiSink::JsonPerRntiSink(const std::string &path) : path_(path) {}

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

  if (format_ == "json_per_rnti") {
    f << "{\"generated_at_s\":" << std::fixed << std::setprecision(6) << now_s
      << ",\"ttl_seconds\":" << ttl_seconds_ << ",\"rntis\":[";
  }

  bool first_rec = true;
  for (const auto &kv : records_) {
    const auto &r = kv.second;
    if (!first_rec) {
      f << ",\n";
    }
    first_rec = false;
    
    if (format_ == "json_per_rnti") {
      f << "{";
    }
    f << "\"rnti\":" << r.rnti << ",";
    
    if (format_ == "json_per_rnti") {
      const bool active = (ttl_seconds_ > 0.0) ? ((now_s - r.last_seen) <= ttl_seconds_) : true;
      f << "\"active\":" << (active ? "true":"false") << ",";
    }
    
    f << "\"first_seen\":" << std::fixed << std::setprecision(6) << r.first_seen << ","
      << "\"last_seen\":" << std::fixed << std::setprecision(6) << r.last_seen << ","
      << "\"first_sample\":" << r.first_sample << ","
      << "\"last_sample\":" << r.last_sample << ","
      << "\"seen_count\":" << r.seen_count << ","
      << "\"cell_id\":" << r.cell_id << ","
      << "\"scrambling_id\":" << r.scrambling_id << ","
      << "\"coreset_id\":" << (int)r.coreset_id << ","
      << "\"aggregation_level\":" << (int)r.aggregation_level << ","
      << "\"candidate_idx\":" << (int)r.candidate_idx << ","
      << "\"slot\":" << (int)r.slot << ","
      << "\"ofdm_symbol\":" << (int)r.ofdm_symbol << ","
      << "\"num_symbols_per_slot\":" << (int)r.num_symbols_per_slot << ","
      << "\"events\":[";
    
    bool first_ev = true;
    for (const auto &ev : r.events) {
      if (!first_ev) f << ",";
      first_ev = false;
      f << "{";
      if (now_s == -1.0) {
        f << "\"type\":\"" << json_escape(ev.type) << "\",";
      }
      f << "\"t\":" << std::fixed << std::setprecision(6) << ev.t_seconds << ","
        << "\"sample\":" << ev.sample_index << ","
        << "\"correlation\":" << ev.correlation << ","
        << "\"slot\":" << (int)ev.slot << ","
        << "\"ofdm_symbol\":" << (int)ev.ofdm_symbol << ","
        << "\"aggregation_level\":" << (int)ev.aggregation_level << ","
        << "\"candidate_idx\":" << (int)ev.candidate_idx
        << "}";
    }
    f << "]}";
  }
  f << "]";
  if (format_ == "json_per_rnti") {
    f << "}";
  }
  f.close();

  std::error_code ec;
  std::filesystem::rename(tmp, path_, ec);
  if (ec) {
    // Fallback su copy+overwrite se rename atomico fallisce su FS non supportato
    std::filesystem::copy_file(tmp, path_, std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::remove(tmp, ec);
  }
}


// ——————————————————— Tracker Initialization ———————————————————
std::unique_ptr<IRntiSink> RntiTracker::make_sink(const std::string &format, const std::string &path) {
  if (format == "json_per_rnti") {
    return std::make_unique<JsonPerRntiSink>(path);
  }
  return std::make_unique<JsonlRntiSink>(path);
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
    r.cell_id = ev.cell_id;
    r.scrambling_id = ev.scrambling_id;
    r.coreset_id = ev.coreset_id;
    r.aggregation_level = ev.aggregation_level;
    r.candidate_idx = ev.candidate_idx;
    r.slot = ev.slot;
    r.ofdm_symbol = ev.ofdm_symbol;
    r.num_symbols_per_slot = ev.num_symbols_per_slot;
    
    // First event
    RntiEventEntry e{};
    e.type = "new";
    e.t_seconds = ev.t_seconds;
    e.sample_index = ev.sample_index;
    e.correlation = ev.correlation;
    e.slot = ev.slot;
    e.ofdm_symbol = ev.ofdm_symbol;
    e.aggregation_level = ev.aggregation_level;
    e.candidate_idx = ev.candidate_idx;
    r.events.push_back(e);

    it = table_.emplace(ev.rnti, std::move(r)).first;
  } else {
    RntiRecord &r = it->second;
    r.last_seen = ev.t_seconds;
    r.last_sample = ev.sample_index;
    r.seen_count += 1;

    // Keep latest context
    r.cell_id = ev.cell_id;
    r.scrambling_id = ev.scrambling_id;
    r.coreset_id = ev.coreset_id;
    r.aggregation_level = ev.aggregation_level;
    r.candidate_idx = ev.candidate_idx;
    r.slot = ev.slot;
    r.ofdm_symbol = ev.ofdm_symbol;
    r.num_symbols_per_slot = ev.num_symbols_per_slot;

    // Next event
    RntiEventEntry e{};
    e.type = "update";
    e.t_seconds = ev.t_seconds;
    e.sample_index = ev.sample_index;
    e.correlation = ev.correlation;
    e.slot = ev.slot;
    e.ofdm_symbol = ev.ofdm_symbol;
    e.aggregation_level = ev.aggregation_level;
    e.candidate_idx = ev.candidate_idx;
    r.events.push_back(e);
  }

  if (sink_) {
    sink_->on_event(is_new ? "new" : "update", it->second);
  }
}

void RntiTracker::expire_older_than(double cutoff_s) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto it = table_.begin(); it != table_.end();) {
    if (it->second.last_seen < cutoff_s) {
      it = table_.erase(it);
    } else {
      ++it;
    }
  }
}

size_t RntiTracker::active_count(double now_s) const {
  std::lock_guard<std::mutex> lk(mu_);
  size_t n = 0;
  for (const auto &kv : table_) {
    if (now_s - kv.second.last_seen <= ttl_seconds_)
      n++;
  }
  return n;
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
