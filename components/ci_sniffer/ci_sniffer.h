#pragma once
#include "esphome.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace ci_sniffer {

class CISnifferComponent : public Component, public uart::UARTDevice {
 public:
  explicit CISnifferComponent(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void set_text_sensor(text_sensor::TextSensor *sensor) { this->sensor_ = sensor; }

  void setup() override;
  void loop() override;

 protected:
  text_sensor::TextSensor *sensor_{nullptr};
  std::string buffer_;

  std::string format_byte_(uint8_t byte);
};

}  // namespace ci_sniffer
}  // namespace esphome