#pragma once

#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

static const uint8_t MCP4725_ADDR = 0x60;
static const uint8_t MCP4725_RES = 12;

namespace esphome {
namespace mcp4725 {
class MCP4725 : public Component, public output::FloatOutput, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void write_state(float state) override;
  void loop() override;

  // Read 3 bytes from MCP4725 (register + EEPROM). If success, *dac_value gets 12-bit value (0..4095).
  bool read_eeprom(uint16_t *dac_value = nullptr, bool wait_ready = false, uint8_t max_attempts = 20, uint16_t delay_ms = 5);

  // Configuration setters (from output.py)
  void set_save_to_eeprom(bool save) { this->save_to_eeprom_ = save; }
  void set_save_threshold(float t) { this->save_threshold_ = t; }  // relative (0..1)
  void set_save_debounce_ms(uint32_t ms) { this->save_debounce_ms_ = ms; }
  void set_save_min_interval_s(uint32_t s) { this->save_min_interval_ms_ = s * 1000u; }

 protected:
  enum ErrorCode { NONE = 0, COMMUNICATION_FAILED } error_code_{NONE};

  // EEPROM/write scheduling state
  bool save_to_eeprom_{false};
  float last_persisted_level_{-1.0f};        // last value known to be in EEPROM (0..1), -1 = unknown
  float save_threshold_{0.01f};              // relative threshold (default 1%)
  uint32_t save_debounce_ms_{3000};          // default 3s debounce
  uint32_t save_min_interval_ms_{60000};     // default min interval 60s
  uint32_t last_eeprom_write_ms_{0};         // last time we issued EEPROM write
  bool pending_save_{false};
  uint32_t pending_save_ms_{0};
  float pending_save_level_{0.0f};
};

}  // namespace mcp4725
}  // namespace esphome
