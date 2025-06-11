#include "ci_sniffer.h"
#include "esphome/core/log.h"

#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
namespace esphome {
namespace ci_sniffer {

static const char *const TAG = "ci_sniffer";

void CISnifferComponent::setup() {
  ESP_LOGI(TAG, "CI Sniffer gestartet (UART %d)", this->uart_->get_hw_serial_number());
  this->set_timeout(100, [this]() {
    this->start_reading_();
  });
}

void CISnifferComponent::start_reading_() {
  if (!this->available()) return;

  while (this->available() >= 3) {
    // Versuche Break zu erkennen
    uint8_t byte = this->peek();
    if (byte == 0x00) {
      // mögliches Break: viele 0-Bits → Start neuer Frame?
      ESP_LOGD(TAG, "Möglicher Break erkannt (0x00)");
      this->read(); // Byte löschen

      // Sync
      if (this->available() >= 1 && this->peek() == 0x55) {
        this->read();
        if (this->available() >= 1) {
          uint8_t pid = this->read();

          std::vector<uint8_t> data;
          while (this->available() > 1 && data.size() < 8) {
            data.push_back(this->read());
          }
          uint8_t cs = this->read();

          char msg[128];
          snprintf(msg, sizeof(msg), "PID=0x%02X Data=[", pid);
          std::string frame = msg;
          for (auto b : data) {
            sprintf(msg, "%02X ", b);
            frame += msg;
          }
          sprintf(msg, "] CS=0x%02X", cs);
          frame += msg;

          ESP_LOGI(TAG, "%s", frame.c_str());
          if (sensor_ != nullptr)
            sensor_->publish_state(frame);
        }
      }
    } else {
      this->read();  // Müll/alte Bytes verwerfen
    }
  }

  this->set_timeout(10, [this]() {
    this->start_reading_();
  });
}
ESP_LOGI("TEST", "Hallo von der neuen Version!");

}  // namespace ci_sniffer
}  // namespace esphome