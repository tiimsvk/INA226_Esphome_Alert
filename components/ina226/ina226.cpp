#include "ina226.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>
#include <cmath>

namespace esphome {
namespace ina226 {

static const char *const TAG = "ina226";

// | A0   | A1   | Address |
// | GND  | GND  | 0x40    |
// | GND  | V_S+ | 0x41    |
// | GND  | SDA  | 0x42    |
// | GND  | SCL  | 0x43    |
// | V_S+ | GND  | 0x44    |
// | V_S+ | V_S+ | 0x45    |
// | V_S+ | SDA  | 0x46    |
// | V_S+ | SCL  | 0x47    |
// | SDA  | GND  | 0x48    |
// | SDA  | V_S+ | 0x49    |
// | SDA  | SDA  | 0x4A    |
// | SDA  | SCL  | 0x4B    |
// | SCL  | GND  | 0x4C    |
// | SCL  | V_S+ | 0x4D    |
// | SCL  | SDA  | 0x4E    |
// | SCL  | SCL  | 0x4F    |

// ¦¦ Adresy registrov ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦
static const uint8_t INA226_REGISTER_CONFIG       = 0x00;
static const uint8_t INA226_REGISTER_SHUNT_VOLTAGE = 0x01;
static const uint8_t INA226_REGISTER_BUS_VOLTAGE   = 0x02;
static const uint8_t INA226_REGISTER_POWER         = 0x03;
static const uint8_t INA226_REGISTER_CURRENT       = 0x04;
static const uint8_t INA226_REGISTER_CALIBRATION   = 0x05;
static const uint8_t INA226_REGISTER_MASK_ENABLE   = 0x06;
static const uint8_t INA226_REGISTER_ALERT_LIMIT   = 0x07;

// Tabu¾ky pre dump_config
static const uint16_t INA226_ADC_TIMES[]       = {140, 204, 332, 588, 1100, 2116, 4156, 8244};
static const uint16_t INA226_ADC_AVG_SAMPLES[] = {1, 4, 16, 64, 128, 256, 512, 1024};

// Názvy alert funkcií - musia presne zodpoveda hodnotám v sensor.py
static const char *const ALERT_OPTION_NONE = "None";
static const char *const ALERT_OPTION_SOL  = "Shunt Over-Limit";
static const char *const ALERT_OPTION_SUL  = "Shunt Under-Limit";
static const char *const ALERT_OPTION_BOL  = "Bus Over-Limit";
static const char *const ALERT_OPTION_BUL  = "Bus Under-Limit";
static const char *const ALERT_OPTION_POL  = "Power Over-Limit";

// ===============================================================================
// INA226Component::setup()
// ===============================================================================
void INA226Component::setup() {
  // 1) Reset zariadenia
  ConfigurationRegister config;
  config.raw = 0;
  config.reset = 1;
  if (!this->write_byte_16(INA226_REGISTER_CONFIG, config.raw)) {
    this->mark_failed();
    return;
  }
  delay(1);  // Èakaj na dokonèenie resetu

  // 2) Nastav konfiguráciu
  config.raw                          = 0;
  config.reserved                     = 0b100;  // pod¾a datasheetu
  config.avg_samples                  = this->adc_avg_samples_;
  config.bus_voltage_conversion_time  = this->adc_time_voltage_;
  config.shunt_voltage_conversion_time= this->adc_time_current_;
  config.mode                         = 0b111;  // Shunt and Bus, Continuous

  if (!this->write_byte_16(INA226_REGISTER_CONFIG, config.raw)) {
    this->mark_failed();
    return;
  }

  // 3) Vypoèítaj a zapíš kalibráciu
  // LSB prúdu = max_current / 2^15, uložené v mikroampéroch
  uint32_t lsb = static_cast<uint32_t>(
      ceilf(this->max_current_a_ * 1000000.0f / 32768.0f));
  this->calibration_lsb_ = lsb;

  // CAL = 0.00512 / (current_lsb_A * R_shunt_ohm)
  uint32_t calibration = static_cast<uint32_t>(
      0.00512f / (static_cast<float>(lsb) / 1000000.0f * this->shunt_resistance_ohm_));

  ESP_LOGV(TAG, "  LSB=%" PRIu32 " µA, Calibration=%" PRIu32, lsb, calibration);

  if (!this->write_byte_16(INA226_REGISTER_CALIBRATION, calibration)) {
    this->mark_failed();
    return;
  }

  // 4) Inicializuj alert registre do bezpeèného stavu
  if (!this->write_byte_16(INA226_REGISTER_MASK_ENABLE, 0x0000)) {
    this->mark_failed();
    return;
  }
  if (!this->write_byte_16(INA226_REGISTER_ALERT_LIMIT, 0x0000)) {
    this->mark_failed();
    return;
  }
}

// ===============================================================================
// INA226Component::dump_config()
// ===============================================================================
void INA226Component::dump_config() {
  ESP_LOGCONFIG(TAG, "INA226:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    return;
  }

  ESP_LOGCONFIG(TAG, "  Shunt Resistance: %.4f ?", this->shunt_resistance_ohm_);
  ESP_LOGCONFIG(TAG, "  Max Current:      %.3f A",  this->max_current_a_);
  ESP_LOGCONFIG(TAG, "  Current LSB:      %" PRIu32 " µA", this->calibration_lsb_);
  ESP_LOGCONFIG(TAG, "  ADC Time Voltage: %d µs",
                INA226_ADC_TIMES[this->adc_time_voltage_ & 0b111]);
  ESP_LOGCONFIG(TAG, "  ADC Time Current: %d µs",
                INA226_ADC_TIMES[this->adc_time_current_ & 0b111]);
  ESP_LOGCONFIG(TAG, "  ADC Averaging:    %d samples",
                INA226_ADC_AVG_SAMPLES[this->adc_avg_samples_ & 0b111]);
  ESP_LOGCONFIG(TAG, "  Shutdown:         %s", this->shutdown_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Alert Function:   %s", this->alert_function_.c_str());
  ESP_LOGCONFIG(TAG, "  Alert Limit:      %.4f", this->alert_limit_);

  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Bus Voltage",   this->bus_voltage_sensor_);
  LOG_SENSOR("  ", "Shunt Voltage", this->shunt_voltage_sensor_);
  LOG_SENSOR("  ", "Current",       this->current_sensor_);
  LOG_SENSOR("  ", "Power",         this->power_sensor_);
}

// ===============================================================================
// INA226Component::update()
// ===============================================================================
void INA226Component::update() {
  // V shutdown móde neèítame merania
  if (this->shutdown_)
    return;

  if (this->bus_voltage_sensor_ != nullptr) {
    uint16_t raw;
    if (!this->read_byte_16(INA226_REGISTER_BUS_VOLTAGE, &raw)) {
      this->status_set_warning();
      return;
    }
    // Bus voltage je vždy kladný, LSB = 1.25 mV
    float voltage = static_cast<float>(this->twos_complement_(raw, 16)) * 0.00125f;
    this->bus_voltage_sensor_->publish_state(voltage);
  }

  if (this->shunt_voltage_sensor_ != nullptr) {
    uint16_t raw;
    if (!this->read_byte_16(INA226_REGISTER_SHUNT_VOLTAGE, &raw)) {
      this->status_set_warning();
      return;
    }
    // Shunt voltage môže by záporný (dvojkový doplnok), LSB = 2.5 µV
    float voltage = static_cast<float>(this->twos_complement_(raw, 16)) * 0.0000025f;
    this->shunt_voltage_sensor_->publish_state(voltage);
  }

  if (this->current_sensor_ != nullptr) {
    uint16_t raw;
    if (!this->read_byte_16(INA226_REGISTER_CURRENT, &raw)) {
      this->status_set_warning();
      return;
    }
    // Prúd v ampéroch: raw * LSB [µA] / 1 000 000
    float current = static_cast<float>(this->twos_complement_(raw, 16))
                    * (static_cast<float>(this->calibration_lsb_) / 1000000.0f);
    this->current_sensor_->publish_state(current);
  }

  if (this->power_sensor_ != nullptr) {
    uint16_t raw;
    if (!this->read_byte_16(INA226_REGISTER_POWER, &raw)) {
      this->status_set_warning();
      return;
    }
    // Výkon v wattoch: raw * 25 * LSB [µA] / 1 000 000
    // Power LSB = 25 × Current_LSB (pod¾a datasheetu sekcia 6.5.1)
    float power = static_cast<float>(raw)
                  * 25.0f
                  * (static_cast<float>(this->calibration_lsb_) / 1000000.0f);
    this->power_sensor_->publish_state(power);
  }

  this->status_clear_warning();
}

// ===============================================================================
// Shutdown - zápis do Configuration registra
// MODE bits = 000 alebo 100 › Power-Down
// MODE bits = 111            › Shunt and Bus, Continuous
// ===============================================================================
void INA226Component::set_shutdown(bool shutdown) {
  this->shutdown_ = shutdown;
  this->update_config_register_();

  // Informuj switch o novom stave (dôležité pre správne zobrazenie v HA)
  if (this->shutdown_switch_ != nullptr) {
    this->shutdown_switch_->publish_state(shutdown);
  }

  ESP_LOGI(TAG, "INA226 shutdown: %s", shutdown ? "OFF (power-down)" : "ON (running)");
}

// ===============================================================================
// Alert function select - zápis do Mask/Enable registra (06h)
// ===============================================================================
void INA226Component::set_alert_function(const std::string &function) {
  this->alert_function_ = function;
  this->update_alert_registers_();

  if (this->alert_select_ != nullptr) {
    this->alert_select_->publish_state(function);
  }

  ESP_LOGI(TAG, "INA226 alert function set to: %s", function.c_str());
}

// ===============================================================================
// Alert limit number - zápis do Alert Limit registra (07h)
// ===============================================================================
void INA226Component::set_alert_limit(float limit) {
  this->alert_limit_ = limit;
  this->update_alert_registers_();

  if (this->alert_limit_number_ != nullptr) {
    this->alert_limit_number_->publish_state(limit);
  }

  ESP_LOGI(TAG, "INA226 alert limit set to: %.4f", limit);
}

// ===============================================================================
// update_config_register_()
// Prepíše Configuration register pod¾a aktuálneho stavu shutdown_
// ===============================================================================
void INA226Component::update_config_register_() {
  ConfigurationRegister config;
  config.raw                           = 0;
  config.reserved                      = 0b100;
  config.avg_samples                   = this->adc_avg_samples_;
  config.bus_voltage_conversion_time   = this->adc_time_voltage_;
  config.shunt_voltage_conversion_time = this->adc_time_current_;

  // MODE bits: 000 = Power-Down, 111 = Shunt and Bus Continuous
  config.mode = this->shutdown_ ? 0b111 : 0b000;

  if (!this->write_byte_16(INA226_REGISTER_CONFIG, config.raw)) {
    ESP_LOGE(TAG, "Chyba zápisu do Configuration registra");
    this->status_set_warning();
  }
}

// ===============================================================================
// update_alert_registers_()
// Zapíše Mask/Enable (06h) a Alert Limit (07h) pod¾a aktuálneho nastavenia
// ===============================================================================
void INA226Component::update_alert_registers_() {
  // 1) Mask/Enable register - nastav alert funkciu
  uint16_t mask = this->alert_function_to_mask_(this->alert_function_);

  if (!this->write_byte_16(INA226_REGISTER_MASK_ENABLE, mask)) {
    ESP_LOGE(TAG, "Chyba zápisu do Mask/Enable registra");
    this->status_set_warning();
    return;
  }

  // 2) Alert Limit register - nastav prahovú hodnotu
  uint16_t raw_limit = this->alert_limit_to_raw_(this->alert_limit_);

  if (!this->write_byte_16(INA226_REGISTER_ALERT_LIMIT, raw_limit)) {
    ESP_LOGE(TAG, "Chyba zápisu do Alert Limit registra");
    this->status_set_warning();
    return;
  }

  ESP_LOGD(TAG, "Alert: mask=0x%04X, raw_limit=0x%04X (%.4f)",
           mask, raw_limit, this->alert_limit_);
}

// ===============================================================================
// alert_function_to_mask_()
// Prevedie názov funkcie na hodnotu pre Mask/Enable register (06h)
// ===============================================================================
uint16_t INA226Component::alert_function_to_mask_(const std::string &function) {
  if (function == ALERT_OPTION_SOL) return static_cast<uint16_t>(ALERT_SOL);
  if (function == ALERT_OPTION_SUL) return static_cast<uint16_t>(ALERT_SUL);
  if (function == ALERT_OPTION_BOL) return static_cast<uint16_t>(ALERT_BOL);
  if (function == ALERT_OPTION_BUL) return static_cast<uint16_t>(ALERT_BUL);
  if (function == ALERT_OPTION_POL) return static_cast<uint16_t>(ALERT_POL);
  return 0x0000;  // None - žiadny alert
}

// ===============================================================================
// alert_limit_to_raw_()
// Prevedie fyzikálnu hodnotu na raw hodnotu pre Alert Limit register (07h)
//
// Pravidlá konverzie (datasheet sekcia 7.1.8):
//   SOL/SUL: Shunt Voltage › LSB = 2.5 µV › raw = limit_V / 0.0000025
//   BOL/BUL: Bus Voltage   › LSB = 1.25 mV › raw = limit_V / 0.00125
//   POL:     Power         › Power_LSB = 25 × Current_LSB [µA] / 1e6
//            raw = limit_W / (25 × lsb_A)
//   None:    raw = 0
// ===============================================================================
uint16_t INA226Component::alert_limit_to_raw_(float limit) {
  if (this->alert_function_ == ALERT_OPTION_SOL ||
      this->alert_function_ == ALERT_OPTION_SUL) {
    // Shunt voltage: vstup v [V], LSB = 2.5 µV
    return static_cast<uint16_t>(limit / 0.0000025f);

  } else if (this->alert_function_ == ALERT_OPTION_BOL ||
             this->alert_function_ == ALERT_OPTION_BUL) {
    // Bus voltage: vstup v [V], LSB = 1.25 mV
    return static_cast<uint16_t>(limit / 0.00125f);

  } else if (this->alert_function_ == ALERT_OPTION_POL) {
    // Power: vstup v [W]
    // Power_LSB = 25 × Current_LSB [µA] / 1 000 000
    float power_lsb = 25.0f * static_cast<float>(this->calibration_lsb_) / 1000000.0f;
    return static_cast<uint16_t>(limit / power_lsb);
  }

  return 0x0000;
}

// ===============================================================================
// twos_complement_()
// ===============================================================================
int32_t INA226Component::twos_complement_(int32_t val, uint8_t bits) {
  if (val & (static_cast<uint32_t>(1) << (bits - 1))) {
    val -= static_cast<uint32_t>(1) << bits;
  }
  return val;
}

// ===============================================================================
// INA226ShutdownSwitch::write_state()
// Volaná z Home Assistant keï používate¾ prepne switch
// ===============================================================================
void INA226ShutdownSwitch::write_state(bool state) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_shutdown(state);
  // publish_state je volané v set_shutdown() › nie tu, aby sme predišli
  // dvojitému volaniu
}

// ===============================================================================
// INA226AlertSelect::control()
// Volaná z Home Assistant keï používate¾ vyberie alert funkciu
// ===============================================================================
void INA226AlertSelect::control(const std::string &value) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_alert_function(value);
}

// ===============================================================================
// INA226AlertNumber::control()
// Volaná z Home Assistant keï používate¾ zmení alert limit
// ===============================================================================
void INA226AlertNumber::control(float value) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_alert_limit(value);
}

}  // namespace ina226
}  // namespace esphome
