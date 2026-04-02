#include "mcp4725.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace mcp4725 {

static const char *const TAG = "mcp4725";

void MCP4725::setup() {
  // základná I2C kontrola
  auto err = this->write(nullptr, 0);
  if (err != i2c::ERROR_OK) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }

  // Pokúsime sa prečítať aktuálnu DAC hodnotu (register/EEPROM) a publikovať ju.
  // Použijeme retry, aby sme čakali, kým nie je EEPROM zapísaná (RDY bit).
  uint16_t dac_raw;
  if (this->read_eeprom(&dac_raw, true, 20, 5)) {
    const float level = (float)dac_raw / (pow(2, MCP4725_RES) - 1);
    ESP_LOGD(TAG, "Read MCP4725 raw=%u -> level=%f", dac_raw, level);
    this->publish_state(level);
  } else {
    ESP_LOGW(TAG, "Could not read MCP4725 EEPROM/register on startup or device busy");
  }
}

void MCP4725::dump_config() {
  LOG_I2C_DEVICE(this);

  if (this->error_code_ == COMMUNICATION_FAILED) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
}

// write_state: buď len do DAC register (volatile, command 0x40) alebo aj do EEPROM (non-volatile, 0x60)
void MCP4725::write_state(float state) {
  const uint16_t value = (uint16_t) round(state * (pow(2, MCP4725_RES) - 1));
  uint8_t command = this->save_to_eeprom_ ? 0x60 : 0x40;
  this->write_byte_16(command, value << 4);
}

bool MCP4725::read_eeprom(uint16_t *dac_value, bool wait_ready, uint8_t max_attempts, uint16_t delay_ms) {
  uint8_t data[3];

  for (uint8_t attempt = 0; attempt < max_attempts; ++attempt) {
    auto err = this->read(data, 3);
    if (err != i2c::ERROR_OK) {
      this->error_code_ = COMMUNICATION_FAILED;
      ESP_LOGW(TAG, "I2C read failed (attempt %u)", attempt + 1);
      return false;
    }

    // RDY bit je MSB prvého bytu: 1 = busy (EEPROM write in progress), 0 = ready
    const bool busy = (data[0] & 0x80);
    if (busy) {
      if (!wait_ready) {
        ESP_LOGW(TAG, "MCP4725 reports RDY=1 (EEPROM busy), not waiting (read_eeprom called without wait_ready).");
        return false;
      }
      // ak máme ďalšie pokusy, počkáme a skúšame znovu
      if (attempt + 1 < max_attempts) {
        delay(delay_ms);
        continue;
      } else {
        ESP_LOGW(TAG, "MCP4725 still busy after %u attempts", max_attempts);
        return false;
      }
    }

    // Ak tu sme, device ready -> parse 12-bit DAC value:
    // Byte0: xxxx D11 D10 D9 D8  (D11..D8 sú v nízkych 4 bitoch)
    // Byte1: D7..D0
    uint16_t raw = (uint16_t)(data[0] & 0x0F) << 8;
    raw |= (uint16_t)data[1];

    if (dac_value)
      *dac_value = raw;

    return true;
  }

  // teoreticky sa sem nedostaneme, ale pre istotu:
  return false;
}

}  // namespace mcp4725
}  // namespace esphome