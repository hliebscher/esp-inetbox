#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace ci_sniffer {

class CISnifferComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;

  void set_sensor(text_sensor::TextSensor *sensor) { sensor_ = sensor; }

 protected:
  void start_reading_();  // ✅ Diese Zeile muss da sein

  text_sensor::TextSensor *sensor_{nullptr};
};

}  // namespace ci_sniffer
}  // namespace esphome