#pragma once
#include "esphome.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace lin_sniffer {

class LINSnifferComponent : public Component, public uart::UARTDevice {
 public:
  explicit LINSnifferComponent(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void set_text_sensor(text_sensor::TextSensor *sensor) { this->sensor_ = sensor; }

  void setup() override {
    ESP_LOGI("lin_sniffer", "LIN Sniffer gestartet");
    state_ = WAIT_BREAK;
  }

  void loop() override {
    uint32_t now = millis();
    while (available()) {
      uint8_t byte = read();
      uint32_t time_since_last = now - last_byte_time_;
      last_byte_time_ = now;

      if (time_since_last > break_timeout_ms_) {
        state_ = WAIT_SYNC;
        current_frame_.clear();
        current_frame_.push_back(byte);
        continue;
      }

      current_frame_.push_back(byte);

      if (state_ == WAIT_SYNC && byte == 0x55) {
        state_ = WAIT_PID;
      } else if (state_ == WAIT_PID && current_frame_.size() >= 3) {
        state_ = READ_DATA;
        data_bytes_read_ = 0;
      } else if (state_ == READ_DATA) {
        data_bytes_read_++;
        if (data_bytes_read_ >= max_data_length_) {
          state_ = DONE;
        }
      }

      if (state_ == DONE || current_frame_.size() > 12) {
        decode_and_log_frame_();
        state_ = WAIT_BREAK;
        current_frame_.clear();
      }
    }
  }

 protected:
  enum State {
    WAIT_BREAK,
    WAIT_SYNC,
    WAIT_PID,
    READ_DATA,
    DONE
  } state_ = WAIT_BREAK;

  std::vector<uint8_t> current_frame_;
  uint32_t last_byte_time_ = 0;
  uint32_t break_timeout_ms_ = 5;
  int max_data_length_ = 8;
  int data_bytes_read_ = 0;

  text_sensor::TextSensor *sensor_{nullptr};

  void decode_and_log_frame_() {
    if (current_frame_.size() < 3) return;

    uint8_t pid = current_frame_[2];
    uint8_t id = pid & 0x3F;
    std::vector<uint8_t> data(current_frame_.begin() + 3, current_frame_.end() - 1);
    uint8_t checksum = current_frame_.back();

    std::string str;
    for (auto b : data) {
      char buf[4];
      sprintf(buf, "%02X ", b);
      str += buf;
    }

    std::string label = describe_frame_id(id);

    ESP_LOGI("lin_sniffer", "Frame: ID=0x%02X (%s) Data=[%s] CS=0x%02X", id, label.c_str(), str.c_str(), checksum);

    if (sensor_ != nullptr) {
      sensor_->publish_state("ID=" + to_string(id) + " [" + str + "] " + label);
    }

    decode_frame_content_(id, data);
  }

  void decode_frame_content_(uint8_t id, const std::vector<uint8_t> &data) {
    if (id == 0x1B && data.size() >= 6) {
      int zone1 = decode_temp(data[0]);
      int zone2 = decode_temp(data[1]);
      int outdoor = decode_temp(data[2]);
      ESP_LOGI("lin_decode", "Temp Z1: %d°C, Z2: %d°C, Outdoor: %d°C", zone1, zone2, outdoor);
    }
  }

  std::string describe_frame_id(uint8_t id) {
    switch (id) {
      case 0x1A: return "Control Frame";
      case 0x1B: return "Info Frame";
      case 0x03: return "Heating Command";
      case 0x04: return "Boiler Setting";
      case 0x05: return "Heating Mode";
      case 0x06: return "Electric Power";
      case 0x07: return "Current Temp Report";
      case 0x13: return "Pump Report";
      case 0x15: return "Energy Usage";
      case 0x16: return "Alde Status";
      case 0x3C: return "Unknown Frame (3C)";
      default: return "Unknown";
    }
  }

  int decode_temp(uint8_t b) {
    if (b <= 0xFA) return b - 0x2A;
    if (b == 0xFB) return -100;
    if (b == 0xFC) return 100;
    if (b == 0xFD) return -999;
    return -998;
  }
};

}  // namespace lin_sniffer
}  // namespace esphome