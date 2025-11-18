#include "rnti_tracker.hpp"
#include <cstdio>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <cstring>


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


// —————————————————————————— CsvPerEventSink ——————————————————————————
CsvPerEventSink::CsvPerEventSink(const std::string &path) : path_(path + ".csv") {}

void CsvPerEventSink::ensure_csv_header() {
  std::error_code ec;
  bool need_header = false;
  if (!std::filesystem::exists(path_, ec)) {
    need_header = true;
  } else {
    auto sz = std::filesystem::file_size(path_, ec);
    need_header = ec || sz == 0;
  }
  if (need_header) {
    std::ofstream f(path_, std::ios::app);
    f << "t_seconds,rnti,cell_id,scrambling_id,coreset_id,aggregation_level,candidate_idx,slot,ofdm_symbol,correlation,sample_index,seen_count,revivals\n";
  }
}

void CsvPerEventSink::append_csv_event(const RntiEvent& ev, const RntiRecord& r) {
  std::ofstream f(path_, std::ios::app);
  f << std::fixed << std::setprecision(6) << ev.t_seconds << ","
    << ev.rnti << ","
    << ev.cell_id << ","
    << ev.scrambling_id << ","
    << static_cast<int>(ev.coreset_id) << ","
    << static_cast<int>(ev.aggregation_level) << ","
    << static_cast<int>(ev.candidate_idx) << ","
    << static_cast<int>(ev.slot) << ","
    << static_cast<int>(ev.ofdm_symbol) << ","
    << ev.correlation << ","
    << ev.sample_index << ","
    << r.seen_count << ","
    << r.revivals
    << "\n";
}

void CsvPerEventSink::on_event(const std::string &/*event_type*/, const RntiRecord &r) {
  if (r.events.empty()) return;
  ensure_csv_header();
  const RntiEvent& ev = r.events.back();
  append_csv_event(ev, r);
}

// ————————————————————————— CompositeRntiSink —————————————————————————
void CompositeRntiSink::on_event(const std::string &event_type, const RntiRecord &rec) {
  for (auto& s : sinks_) s->on_event(event_type, rec);
}

void CompositeRntiSink::flush() {
  for (auto& s : sinks_) s->flush();
}

void CompositeRntiSink::set_config(const std::string& format, double ttl_seconds) {
  for (auto& s : sinks_) {
    if (auto* js = dynamic_cast<JsonPerRntiSink*>(s.get())) {
      js->set_config(format, ttl_seconds);
    } else if (auto* cs = dynamic_cast<CompositeRntiSink*>(s.get())) {
      cs->set_config(format, ttl_seconds);
    }
  }
}


// ————————————————————————— ZmqRntiSink —————————————————————————

ZmqSink::ZmqSink(const std::string &endpoint, bool bind) : endpoint_(endpoint), bind_(bind) {
  ctx_ = zmq_ctx_new();
  sock_ = zmq_socket(ctx_, ZMQ_PUB);

  int hwm = 100000;
  zmq_setsockopt(sock_, ZMQ_SNDHWM, &hwm, sizeof(hwm));

  int immediate = 1;
  zmq_setsockopt(sock_, ZMQ_IMMEDIATE, &immediate, sizeof(immediate));

  if (bind_) {
    if (zmq_bind(sock_, endpoint_.c_str()) != 0) {
      std::fprintf(stderr, "[ZmqSink] zmq_bind failed: %s\n", zmq_strerror(zmq_errno()));
    }
  } else {
    if (zmq_connect(sock_, endpoint_.c_str()) != 0) {
      std::fprintf(stderr, "[ZmqSink] zmq_connect failed: %s\n", zmq_strerror(zmq_errno()));
    }
  }
}

ZmqSink::~ZmqSink() {
  if (sock_) zmq_close(sock_);
  if (ctx_) zmq_ctx_term(ctx_);
}

void ZmqSink::on_event(const std::string &event_type, const RntiRecord &r) {
  if (!sock_) return;
  if (r.events.empty()) return;  // nessun evento da pubblicare

  const RntiEvent& ev = r.events.back();

  char buf[4096];
  std::snprintf(buf, sizeof(buf),
    "{"
      "\"type\":\"rnti_event\","
      "\"event\":\"%s\","
      "\"rnti\":%u,"
      "\"cell_id\":%u,"
      "\"scrambling_id\":%u,"
      "\"coreset_id\":%u,"
      "\"t_seconds\":%.6f,"
      "\"sample_index\":%lld,"
      "\"seen_count\":%u,"
      "\"revivals\":%u,"
      "\"aggregation_level\":%u,"
      "\"candidate_idx\":%u,"
      "\"slot\":%u,"
      "\"ofdm_symbol\":%u,"
      "\"correlation\":%.6f,"
      "\"status\":\"active\""
    "}",
    event_type.c_str(),
    static_cast<unsigned>(r.rnti),
    static_cast<unsigned>(ev.cell_id),
    static_cast<unsigned>(ev.scrambling_id),
    static_cast<unsigned>(ev.coreset_id),
    ev.t_seconds,
    static_cast<long long>(ev.sample_index),
    static_cast<unsigned>(r.seen_count),
    static_cast<unsigned>(r.revivals),
    static_cast<unsigned>(ev.aggregation_level),
    static_cast<unsigned>(ev.candidate_idx),
    static_cast<unsigned>(ev.slot),
    static_cast<unsigned>(ev.ofdm_symbol),
    static_cast<double>(ev.correlation)
  );

  zmq_send(sock_, buf, std::strlen(buf), ZMQ_DONTWAIT);
}



// ——————————————————— Tracker Initialization ———————————————————

std::unique_ptr<IRntiSink> RntiTracker::make_sink(const std::string &format, const std::string &path) {
  const std::string f = format;
  const bool want_json = (f.find("json") != std::string::npos) || f.empty();
  const bool want_csv  = (f.find("csv") != std::string::npos);
  const bool want_zmq  = (f.find("zmq") != std::string::npos);

  if (want_json && want_csv && want_zmq) {
    auto composite = std::make_unique<CompositeRntiSink>();
    composite->add_sink(std::make_unique<JsonPerRntiSink>(path));
    composite->add_sink(std::make_unique<CsvPerEventSink>(path));
    composite->add_sink(std::make_unique<ZmqSink>("tcp://*:5557", true));
    return composite;
  }
  if (want_json && want_csv) {
    auto composite = std::make_unique<CompositeRntiSink>();
    composite->add_sink(std::make_unique<JsonPerRntiSink>(path));
    composite->add_sink(std::make_unique<CsvPerEventSink>(path));
    return composite;
  } else if (want_json) {
    return std::make_unique<JsonPerRntiSink>(path);
  } else if (want_csv) {
    return std::make_unique<CsvPerEventSink>(path);
  }
  if (want_zmq) {
    return std::make_unique<ZmqSink>("tcp://*:5557", true);
  }
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
  } else if (auto *m = dynamic_cast<CompositeRntiSink*>(sink_.get())) {
    m->set_config(format, ttl_seconds_);
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

    // O(1): new RNTI inserted -> increment active counter
    active_count_++;

  } else {
    RntiRecord &r = it->second;

    const double gap = ev.t_seconds - r.last_seen;
    if (gap > ttl_seconds_) {
      r.revivals++;
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

// To keep a counter of active RNTIs in O(1)
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
