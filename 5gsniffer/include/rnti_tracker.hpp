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
  bool crc_ok = true;
  float correlation = 0.f;
  int64_t sample_index = 0;
  double t_seconds = 0.0;
};

struct RntiEventEntry {
  std::string type; // "new" | "update"
  double t_seconds = 0.0;
  int64_t sample_index = 0;
  float correlation = 0.f;
  uint8_t slot = 0;
  uint8_t ofdm_symbol = 0;
  uint8_t aggregation_level = 0;
  uint8_t candidate_idx = 0;
};

struct RntiRecord {
  uint16_t rnti = 0;
  double first_seen = 0.0;
  double last_seen = 0.0;
  int64_t first_sample = 0;
  int64_t last_sample = 0;
  uint32_t seen_count = 0;

  uint16_t cell_id = 0;
  uint16_t scrambling_id = 0;
  uint8_t coreset_id = 0;
  uint8_t aggregation_level = 0;
  uint8_t candidate_idx = 0;
  uint8_t slot = 0;
  uint8_t ofdm_symbol = 0;
  uint8_t num_symbols_per_slot = 14;

  std::vector<RntiEventEntry> events; // Historic events for this RNTI
};

/** Sink interface for logging. */
class IRntiSink {
public:
  virtual ~IRntiSink() = default;
  virtual void on_event(const std::string &event_type, const RntiRecord &rec) = 0;
  virtual void flush() {}
};

// ———————————————————— JSON events per line ————————————————————
class JsonlRntiSink : public IRntiSink {
public:
  explicit JsonlRntiSink(const std::string &path);
  void on_event(const std::string &event_type, const RntiRecord &rec) override;

private:
  std::string path_;
};


// ——————————————————————— JSON per RNTI ———————————————————————
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
class RntiTracker {
public:
  static RntiTracker &instance();
  void configure(const std::string &output_path,
                 const std::string &format,
                 double ttl_seconds);

  void observe(const RntiEvent &ev);       // Feed a new observation (CRC-valid DCI -> mapped to an RNTI)
  void expire_older_than(double cutoff_s); // Maintenance: drop stale entries (not seen within TTL)
  size_t active_count(double now_s) const; // Currently active device-count
  std::optional<RntiRecord> get(uint16_t rnti) const;

  void flush();
  double ttl_seconds() const { return ttl_seconds_; }

private:
  RntiTracker() = default;

  mutable std::mutex mu_;
  std::unordered_map<uint16_t, RntiRecord> table_;
  std::unique_ptr<IRntiSink> sink_;
  double ttl_seconds_;

  static std::unique_ptr<IRntiSink> make_sink(const std::string &format, const std::string &path);
};
