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

  // Prečíta 3-byty z MCP4725 (register + EEPROM).
  // Ak success, *dac_value obsahuje 12-bit aktuálnu DAC hodnotu (0..4095).
  // wait_ready - ak true, bude sa opakovane čítať (a čakať) až do max_attempts, kým zariadenie nie je ready.
  // max_attempts, delay_ms - retry parametre (použité len keď wait_ready=true).
  bool read_eeprom(uint16_t *dac_value = nullptr, bool wait_ready = false, uint8_t max_attempts = 20, uint16_t delay_ms = 5);

  // Nastaví, či zapisovať do EEPROM pri každom sete (false = iba DAC register, volatile).
  void set_save_to_eeprom(bool save) { this->save_to_eeprom_ = save; }

 protected:
  enum ErrorCode { NONE = 0, COMMUNICATION_FAILED } error_code_{NONE};
  bool save_to_eeprom_{false};
};

}  // namespace mcp4725
}  // namespace esphome