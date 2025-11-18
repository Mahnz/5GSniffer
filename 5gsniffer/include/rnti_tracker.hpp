#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <fstream>
#include <chrono>
#include <optional>
#include <vector>

#include <zmq.h>

struct RntiEvent {
  uint16_t rnti = 0;
  uint16_t cell_id = 0;
  uint16_t scrambling_id = 0;
  uint8_t coreset_id = 0;
  uint8_t aggregation_level = 0;
  uint8_t candidate_idx = 0;
  uint8_t slot = 0;
  uint8_t ofdm_symbol = 0;
  uint8_t num_symbols_per_slot = 14;
  float correlation = 0.f;
  int64_t sample_index = 0;
  double t_seconds = 0.0;
};

struct RntiRecord {
  uint16_t rnti = 0;
  double first_seen = 0.0;
  double last_seen = 0.0;
  int64_t first_sample = 0;
  int64_t last_sample = 0;
  uint32_t seen_count = 0;       // Number of occurrences observed
  uint32_t revivals = 0;         // Number of times this RNTI goes 'inactive -> active'

  std::vector<RntiEvent> events; // historic events for a specific RNTI
};

class IRntiSink {
public:
  virtual ~IRntiSink() = default;
  virtual void on_event(const std::string &event_type, const RntiRecord &rec) = 0;
  virtual void flush() {}
};

// ——————————————————————————————————————————————————————————————

class JsonPerRntiSink : public IRntiSink {
public:
  explicit JsonPerRntiSink(const std::string &path);
  void on_event(const std::string &event_type, const RntiRecord &rec) override;
  void flush() override;

  void set_config(const std::string& format, double ttl_seconds) {
    format_ = format;
    ttl_seconds_ = ttl_seconds;
  }

private:
  double logical_now() const;   // latest last_seen across records_
  void write_all();

  std::string path_;
  std::string format_;
  double ttl_seconds_ = 0.0;
  std::unordered_map<uint16_t, RntiRecord> records_;
};

// ——————————————————————————————————————————————————————————————

class CsvPerEventSink : public IRntiSink {
public:
  explicit CsvPerEventSink(const std::string &path);
  void on_event(const std::string &event_type, const RntiRecord &rec) override;
  void flush() override {}

private:
  void ensure_csv_header();
  void append_csv_event(const RntiEvent& ev, const RntiRecord& rec);
  std::string path_;
};

class CompositeRntiSink : public IRntiSink {
public:
  void add_sink(std::unique_ptr<IRntiSink> s) { sinks_.emplace_back(std::move(s)); }
  void on_event(const std::string &event_type, const RntiRecord &rec) override;
  void flush() override;
  void set_config(const std::string& format, double ttl_seconds);

private:
  std::vector<std::unique_ptr<IRntiSink>> sinks_;
};

// ——————————————————————————————————————————————————————————————

class ZmqSink : public IRntiSink {
public:
  // endpoint examples: "tcp://*:5557" to bind, or "tcp://broker:5557" to connect
  explicit ZmqSink(const std::string &endpoint, bool bind);
  ~ZmqSink() override;

  void on_event(const std::string &event_type, const RntiRecord &rec) override;
  void flush() override {}

  // optional: allow runtime changes (no-op here)
  void set_config(const std::string&, double) {}

private:
  void *ctx_ = nullptr;
  void *sock_ = nullptr;
  std::string endpoint_;
  bool bind_ = true;
};

// ——————————————————————————————————————————————————————————————

class RntiTracker {
public:
  static RntiTracker &instance();
  void configure(const std::string &output_path,
                 const std::string &format,
                 double ttl_seconds);

  void observe(const RntiEvent &ev);         // Feed a new observation (CRC-valid DCI -> mapped to an RNTI)
  void expire_older_than(double cutoff_s);   // Drop entries not seen within TTL
  size_t active_count(double now_s) const;   // Currently active device-count
  std::optional<RntiRecord> get(uint16_t rnti) const;
  
  void flush();
  double ttl_seconds() const { return ttl_seconds_; }

  private:
  RntiTracker() = default;

  mutable std::mutex mu_;
  std::unordered_map<uint16_t, RntiRecord> table_;
  std::unique_ptr<IRntiSink> sink_;
  double ttl_seconds_ = 0.0;
  size_t active_count_ = 0;    // TEST - Maintain number of active RNTIs [O(1) instead of O(n)]

  static std::unique_ptr<IRntiSink> make_sink(const std::string &format, const std::string &path);
};
