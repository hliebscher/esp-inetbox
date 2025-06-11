#include "ci_sniffer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ci_sniffer {

static const char *const TAG = "ci_sniffer";

void CISnifferComponent::setup() {
  ESP_LOGI(TAG, "CI Sniffer gestartet (19200 Baud)");
}

void CISnifferComponent::loop() {
  while (available()) {
    uint8_t byte = read();
    buffer_ += format_byte_(byte);
    if (buffer_.length() > 100) buffer_.clear();

    if (sensor_ != nullptr)
      sensor_->publish_state(buffer_);
  }
}

std::string CISnifferComponent::format_byte_(uint8_t byte) {
  char buf[4];
  sprintf(buf, "%02X ", byte);
  return std::string(buf);
}

}  // namespace ci_sniffer
}  // namespace esphome