// ci_sniffer.cpp (mit RMT Break Detection)
#include "ci_sniffer.h"
#include "esphome/core/log.h"
#include "driver/rmt_rx.h"

namespace esphome {
namespace ci_sniffer {

static const char *const TAG = "ci_sniffer";

static const gpio_num_t LIN_RX_PIN = GPIO_NUM_14;
static const uint32_t BREAK_THRESHOLD_US = 600;  // 13 bits at 19200 baud ~676us

static rmt_channel_handle_t rmt_rx_channel = nullptr;

static bool frame_started = false;
static uint32_t last_break_time = 0;

void rmt_rx_callback(rmt_channel_handle_t, const rmt_rx_done_event_data_t *edata, void *user_data) {
  for (size_t i = 0; i < edata->num_symbols; i++) {
    const rmt_symbol_word_t &symbol = edata->rx_symbols[i];
    if (symbol.level0 == 0 && symbol.duration0 > BREAK_THRESHOLD_US * 80) {
      last_break_time = millis();
      frame_started = true;
      ESP_LOGD(TAG, "Break detected (%uus)", symbol.duration0 / 80);
    }
  }
}

void CISnifferComponent::setup() {
  ESP_LOGI(TAG, "CI Sniffer mit RMT Break Detection gestartet (GPIO14, 19200 Baud)");

  rmt_receive_config_t rx_config = {
    .signal_range_min_ns = 50000,
    .signal_range_max_ns = 2000000
  };

  rmt_rx_channel_config_t rx_chan_cfg = {
    .gpio_num = LIN_RX_PIN,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 1000000,
    .mem_block_symbols = 64,
    .flags = RMT_CHANNEL_FLAG_IO_LOOP_BACK_ONLY
  };

  ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_chan_cfg, &rmt_rx_channel));
  ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rmt_rx_channel, &(rmt_rx_event_callbacks_t){.on_recv_done = rmt_rx_callback}, nullptr));
  ESP_ERROR_CHECK(rmt_enable_channel(rmt_rx_channel));

  static rmt_symbol_word_t rx_buf[64];
  ESP_ERROR_CHECK(rmt_receive(rmt_rx_channel, rx_buf, sizeof(rx_buf), &rx_config));
}

void CISnifferComponent::loop() {
  if (frame_started && this->available() >= 3) {
    uint8_t sync = this->read();
    if (sync != 0x55) {
      frame_started = false;
      return;
    }
    uint8_t pid = this->read();

    std::vector<uint8_t> data;
    while (this->available() > 1 && data.size() < 8) {
      data.push_back(this->read());
    }
    uint8_t cs = this->read();

    char msg[128];
    snprintf(msg, sizeof(msg), "ID=0x%02X Data=[", pid);
    std::string frame = msg;
    for (auto b : data) {
      char tmp[4];
      sprintf(tmp, "%02X ", b);
      frame += tmp;
    }
    frame += "] CS=0x";
    sprintf(msg, "%02X", cs);
    frame += msg;

    ESP_LOGI(TAG, "%s", frame.c_str());
    if (sensor_ != nullptr)
      sensor_->publish_state(frame);

    frame_started = false;
  }
}

}  // namespace ci_sniffer
}  // namespace esphome