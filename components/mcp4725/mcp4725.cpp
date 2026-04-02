#include "mcp4725.h"
#include "esphome/core/log.h"
#include <cmath>

#ifdef ARDUINO
#include "Arduino.h"  // millis(), delay()
#else
// ESP-IDF / FreeRTOS includes
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace esphome {
namespace mcp4725 {

static const char *const TAG = "mcp4725";

// portable helpers
static inline uint32_t now_ms() {
#ifdef ARDUINO
  return millis();
#else
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
#endif
}
static inline void sleep_ms(uint32_t ms) {
#ifdef ARDUINO
  delay(ms);
#else
  vTaskDelay(pdMS_TO_TICKS(ms));
#endif
}

void MCP4725::setup() {
  // Basic I2C probe: write 0 bytes to check device
  auto err = this->write(nullptr, 0);
  if (err != i2c::ERROR_OK) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }

  // Try to read persisted DAC value (wait for device ready with retries)
  uint16_t dac_raw;
  if (this->read_eeprom(&dac_raw, true, 20, 5)) {
    const float level = (float)dac_raw / (pow(2, MCP4725_RES) - 1);
    ESP_LOGD(TAG, "Read MCP4725 raw=%u -> level=%f", dac_raw, level);
    // Publish to ESPHome/HA via base class method
    this->output::FloatOutput::publish_state(level);
    // Store as last known persisted value
    this->last_persisted_level_ = level;
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

// Immediate volatile write (DAC register) + optional schedule persist to EEPROM
void MCP4725::write_state(float state) {
  const uint16_t value = (uint16_t) round(state * (pow(2, MCP4725_RES) - 1));

  // Always do volatile write for immediate response (command 0x40)
  this->write_byte_16(0x40, value << 4);

  // If not configured to save to EEPROM, skip rest
  if (!this->save_to_eeprom_)
    return;

  // Decide whether we should persist this change:
  bool different;
  if (this->last_persisted_level_ < 0.0f)
    different = true;
  else
    different = (fabs(state - this->last_persisted_level_) >= this->save_threshold_);

  if (!different) {
    // No significant change compared to last persisted value
    return;
  }

  uint32_t now = now_ms();

  // Rate limiting: if last EEPROM write was recent, skip scheduling due to min interval
  if (now < this->last_eeprom_write_ms_ + this->save_min_interval_ms_) {
    ESP_LOGD(TAG, "Skipping EEPROM save scheduling due to rate limit");
    return;
  }

  // Schedule save after debounce_ms
  this->pending_save_ms_ = now + this->save_debounce_ms_;
  this->pending_save_level_ = state;
  this->pending_save_ = true;
  ESP_LOGD(TAG, "Scheduled EEPROM save at %u (in %u ms) for level=%f", this->pending_save_ms_, this->save_debounce_ms_, state);
}

void MCP4725::loop() {
  if (!this->pending_save_)
    return;

  uint32_t now = now_ms();
  if (now < this->pending_save_ms_)
    return;

  // Time to perform EEPROM write
  uint16_t value = (uint16_t) round(this->pending_save_level_ * (pow(2, MCP4725_RES) - 1));

  // Perform non-volatile write command (0x60). This will start EEPROM write and device will be busy shortly.
  auto err = this->write_byte_16(0x60, value << 4);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "EEPROM write failed (I2C error). Will clear pending flag and not retry automatically.");
    // Clear pending to avoid infinite loop; user can re-issue change if needed
    this->pending_save_ = false;
    return;
  }

  this->last_eeprom_write_ms_ = now;
  this->last_persisted_level_ = this->pending_save_level_;
  this->pending_save_ = false;
  ESP_LOGI(TAG, "EEPROM saved level=%f (raw=%u) at %u", this->last_persisted_level_, value, this->last_eeprom_write_ms_);
}

// Read 3 bytes and optionally wait for readiness (RDY bit)
bool MCP4725::read_eeprom(uint16_t *dac_value, bool wait_ready, uint8_t max_attempts, uint16_t delay_ms) {
  uint8_t data[3];

  for (uint8_t attempt = 0; attempt < max_attempts; ++attempt) {
    auto err = this->read(data, 3);
    if (err != i2c::ERROR_OK) {
      this->error_code_ = COMMUNICATION_FAILED;
      ESP_LOGW(TAG, "I2C read failed (attempt %u)", attempt + 1);
      return false;
    }

    // RDY bit is MSB of first byte: 1 = busy (EEPROM write), 0 = ready
    const bool busy = (data[0] & 0x80);
    if (busy) {
      if (!wait_ready) {
        ESP_LOGW(TAG, "MCP4725 reports RDY=1 (EEPROM busy), not waiting (read_eeprom called without wait_ready).");
        return false;
      }
      // if more attempts remain, wait and retry
      if (attempt + 1 < max_attempts) {
        sleep_ms(delay_ms);
        continue;
      } else {
        ESP_LOGW(TAG, "MCP4725 still busy after %u attempts", max_attempts);
        return false;
      }
    }

    // Parse 12-bit DAC value:
    uint16_t raw = (uint16_t)(data[0] & 0x0F) << 8;
    raw |= (uint16_t)data[1];

    if (dac_value)
      *dac_value = raw;

    return true;
  }

  return false;
}

}  // namespace mcp4725
}  // namespace esphome
