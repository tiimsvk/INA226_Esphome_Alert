#include "ina228.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>
#include <cmath>

namespace esphome {
namespace ina228 {

static const char *const TAG = "ina228";
static inline uint16_t be16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }

// ---------------------------------------------------------------------------
// Register adresy INA228
// ---------------------------------------------------------------------------
static const uint8_t INA228_REG_CONFIG          = 0x00;
static const uint8_t INA228_REG_ADC_CONFIG      = 0x01;
static const uint8_t INA228_REG_SHUNT_CAL       = 0x02;
static const uint8_t INA228_REG_SHUNT_TEMPCO    = 0x03;
static const uint8_t INA228_REG_VSHUNT          = 0x04;
static const uint8_t INA228_REG_VBUS            = 0x05;
static const uint8_t INA228_REG_DIETEMP         = 0x06;
static const uint8_t INA228_REG_CURRENT         = 0x07;
static const uint8_t INA228_REG_POWER           = 0x08;
static const uint8_t INA228_REG_ENERGY          = 0x09;
static const uint8_t INA228_REG_DIAG_ALRT       = 0x0B;
static const uint8_t INA228_REG_SOVL            = 0x0C;  // Shunt Over-Voltage Limit
static const uint8_t INA228_REG_SUVL            = 0x0D;  // Shunt Under-Voltage Limit
static const uint8_t INA228_REG_BOVL            = 0x0E;  // Bus Over-Voltage Limit
static const uint8_t INA228_REG_BUVL            = 0x0F;  // Bus Under-Voltage Limit
static const uint8_t INA228_REG_TEMP_LIMIT      = 0x10;  // Temperature Limit
static const uint8_t INA228_REG_PWR_LIMIT       = 0x11;  // Power Limit
static const uint8_t INA228_REG_MANUFACTURER_ID = 0x3E; 
static const uint8_t INA228_REG_DEVICE_ID       = 0x3F; 

// ---------------------------------------------------------------------------
// Tabuľky pre dump_config
// ---------------------------------------------------------------------------
static const uint16_t INA228_ADC_TIMES[] = {50, 84, 150, 280, 540, 1052, 2074, 4120};
static const uint16_t INA228_ADC_AVGS[]  = {1, 4, 16, 64, 128, 256, 512, 1024, 2048, 4096};

// ---------------------------------------------------------------------------
// Názvy alert funkcií
// ---------------------------------------------------------------------------
static const char *const ALERT_OPT_NONE   = "None";
static const char *const ALERT_OPT_SHNTOL = "Shunt Over-Limit";
static const char *const ALERT_OPT_SHNTUL = "Shunt Under-Limit";
static const char *const ALERT_OPT_BUSOL  = "Bus Over-Limit";
static const char *const ALERT_OPT_BUSUL  = "Bus Under-Limit";
static const char *const ALERT_OPT_POL    = "Power Over-Limit";

// ===========================================================================
// setup()
// ===========================================================================
void INA228Component::setup() {
  ESP_LOGD(TAG, "Inicializácia INA228...");

  // 1) Full reset zariadenia
  uint16_t reset_val = 0x8000; // RST bit (bit 15)

  if (!this->write_byte_16(INA228_REG_CONFIG, be16(reset_val))) {  // FIX: zátvorka
    ESP_LOGE(TAG, "Nepodarilo sa vykonať reset INA228");
    this->mark_failed();
    return;
  }

  delay(2);

  // -----------------------------------------------------------------------
  // 2) CONFIG (ADCRANGE + TEMPCOMP)
  // -----------------------------------------------------------------------
  ConfigRegister cfg;
  cfg.raw = 0;
  cfg.adcrange = this->adc_range_ ? 1 : 0;
  cfg.tempcomp = this->temp_comp_enabled_ ? 1 : 0;

  if (!this->write_byte_16(INA228_REG_CONFIG, be16(cfg.raw))) {
    this->mark_failed();
    return;
  }

  // -----------------------------------------------------------------------
  // 2b) SHUNT_TEMPCO
  // -----------------------------------------------------------------------
  if (this->temp_comp_enabled_) {
    int16_t tempco_raw = (int16_t) lroundf(this->shunt_tempco_ppm_);

    if (!this->write_byte_16(INA228_REG_SHUNT_TEMPCO, be16((uint16_t) tempco_raw))) {
      this->mark_failed();
      return;
    }
  }

  // -----------------------------------------------------------------------
  // 3) ADC_CONFIG
  // -----------------------------------------------------------------------
  this->write_adc_config_register_();

  // -----------------------------------------------------------------------
  // 4) Current LSB + SHUNT_CAL
  // -----------------------------------------------------------------------
  this->current_lsb_ = this->max_current_a_ / 524288.0f;

  float shunt_cal_f = 13107.2e6f * this->current_lsb_ * this->shunt_resistance_ohm_;

  if (this->adc_range_) {
    shunt_cal_f *= 4.0f;
  }

  if (shunt_cal_f > 0x7FFF) shunt_cal_f = 0x7FFF;

  uint16_t shunt_cal = (uint16_t) shunt_cal_f;

  if (!this->write_byte_16(INA228_REG_SHUNT_CAL, be16(shunt_cal))) {
    this->mark_failed();
    return;
  }

  // -----------------------------------------------------------------------
  // 5) Reset alert registre
  // -----------------------------------------------------------------------
  this->write_byte_16(INA228_REG_DIAG_ALRT, 0x0000);
  this->write_byte_16(INA228_REG_SOVL, 0x0000);
  this->write_byte_16(INA228_REG_SUVL, 0x0000);
  this->write_byte_16(INA228_REG_BOVL, 0x0000);
  this->write_byte_16(INA228_REG_BUVL, 0x0000);
  this->write_byte_16(INA228_REG_PWR_LIMIT, 0x0000);

  // -----------------------------------------------------------------------
  // 6) Start merania
  // -----------------------------------------------------------------------
  this->shutdown_ = false;
  this->write_adc_config_register_();

  // -----------------------------------------------------------------------
  // 7) publish HA state
  // -----------------------------------------------------------------------
  this->update_alert_registers_();

  if (this->alert_select_ != nullptr)
    this->alert_select_->publish_state(this->alert_function_);

  if (this->alert_limit_number_ != nullptr)
    this->alert_limit_number_->publish_state(this->alert_limit_);

  ESP_LOGI(TAG, "INA228 úspešne nastavený.");
}

// ===========================================================================
// dump_config()
// ===========================================================================
void INA228Component::dump_config() {
  ESP_LOGCONFIG(TAG, "INA228:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    return;
  }

  ESP_LOGCONFIG(TAG, "  Shunt Resistance: %.4f Ω", this->shunt_resistance_ohm_);
  ESP_LOGCONFIG(TAG, "  Max Current:      %.3f A",  this->max_current_a_);
  ESP_LOGCONFIG(TAG, "  Current LSB:      %.6f A",  this->current_lsb_);
  ESP_LOGCONFIG(TAG, "  ADC Range:        %s",
                this->adc_range_ ? "±40.96 mV" : "±163.84 mV");
  uint8_t vt_idx = this->adc_time_voltage_ & 0x07;
  uint8_t ct_idx = this->adc_time_current_ & 0x07;
  uint8_t tt_idx = this->adc_time_temp_    & 0x07;
  uint8_t av_idx = this->adc_avg_samples_  & 0x0F;
  ESP_LOGCONFIG(TAG, "  ADC Time Voltage: %d µs", INA228_ADC_TIMES[vt_idx]);
  ESP_LOGCONFIG(TAG, "  ADC Time Current: %d µs", INA228_ADC_TIMES[ct_idx]);
  ESP_LOGCONFIG(TAG, "  ADC Time Temp:    %d µs", INA228_ADC_TIMES[tt_idx]);
  ESP_LOGCONFIG(TAG, "  ADC Averaging:    %d samples",
                av_idx < 10 ? INA228_ADC_AVGS[av_idx] : 4096);
  ESP_LOGCONFIG(TAG, "  Shutdown:         %s", this->shutdown_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Alert Function:   %s", this->alert_function_.c_str());
  ESP_LOGCONFIG(TAG, "  Alert Limit:      %.4f", this->alert_limit_);

  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Bus Voltage",   this->bus_voltage_sensor_);
  LOG_SENSOR("  ", "Shunt Voltage", this->shunt_voltage_sensor_);
  LOG_SENSOR("  ", "Current",       this->current_sensor_);
  LOG_SENSOR("  ", "Power",         this->power_sensor_);
  LOG_SENSOR("  ", "Temperature",   this->temperature_sensor_);
}

// ===========================================================================
// update()
// ===========================================================================
void INA228Component::update() {
  if (this->shutdown_)
    return;

  // --- Bus Voltage ---
  if (this->bus_voltage_sensor_ != nullptr) {
    uint32_t raw = 0;
    if (!this->read_register_24_(INA228_REG_VBUS, &raw)) {
      this->status_set_warning();
      return;
    }
    // 20-bit unsigned (bity 23..4 sú dáta, bity 3..0 sú vždy 0)
    // LSB = 3.125 mV = 0.000003125 V ... ale presniejšie: 195.3125 µV
    // Podľa datasheetu: VBUS[V] = raw_20bit * 3.125e-3 / 16
    // Čo je ekvivalentné: (raw >> 4) * 195.3125e-6
    int32_t val = static_cast<int32_t>(raw) >> 4;
    float voltage = static_cast<float>(val) * 195.3125e-6f;
    this->bus_voltage_sensor_->publish_state(voltage);
  }

  // --- Shunt Voltage ---
  if (this->shunt_voltage_sensor_ != nullptr) {
    uint32_t raw = 0;
    if (!this->read_register_24_(INA228_REG_VSHUNT, &raw)) {
      this->status_set_warning();
      return;
    }
    // 20-bit signed, ľavé zarovnanie → pravý shift o 4
    int32_t val = this->twos_complement_20_(static_cast<int32_t>(raw) >> 4);
    // LSB závisí od ADCRANGE:
    //   ADCRANGE=0: 312.5 nV = 312.5e-9 V
    //   ADCRANGE=1: 78.125 nV = 78.125e-9 V
    float lsb = this->adc_range_ ? 78.125e-9f : 312.5e-9f;
    float voltage = static_cast<float>(val) * lsb;
    this->shunt_voltage_sensor_->publish_state(voltage);
  }

  // --- Current ---
  if (this->current_sensor_ != nullptr) {
    uint32_t raw = 0;
    if (!this->read_register_24_(INA228_REG_CURRENT, &raw)) {
      this->status_set_warning();
      return;
    }
    // 20-bit signed, ľavé zarovnanie → pravý shift o 4
    int32_t val = this->twos_complement_20_(static_cast<int32_t>(raw) >> 4);
    float current = static_cast<float>(val) * this->current_lsb_;
    this->current_sensor_->publish_state(current);
  }

  // --- Power ---
  if (this->power_sensor_ != nullptr) {
    uint32_t raw = 0;
    if (!this->read_register_24_(INA228_REG_POWER, &raw)) {
      this->status_set_warning();
      return;
    }
    // 24-bit unsigned, žiadny shift
    // Power LSB = 3.2 × Current_LSB (podľa datasheetu)
    float power_lsb = 3.2f * this->current_lsb_;
    float power = static_cast<float>(raw) * power_lsb;
    this->power_sensor_->publish_state(power);
  }

  // --- Die Temperature ---
  if (this->temperature_sensor_ != nullptr) {
    uint16_t raw = 0;
    if (!this->read_byte_16(INA228_REG_DIETEMP, &raw)) {
      this->status_set_warning();
      return;
    }
    
    raw = be16(raw);   // FIX
    
    int16_t val = static_cast<int16_t>(raw);
    float temp = static_cast<float>(val) * 0.0078125f;
    this->temperature_sensor_->publish_state(temp);
  }

  this->status_clear_warning();
}

// ===========================================================================
// set_shutdown()
// state=true  → zariadenie beží (MODE=0b111)
// state=false → power-down     (MODE=0b000)
// ===========================================================================
void INA228Component::set_shutdown(bool state) {
  this->shutdown_ = !state;
  this->write_adc_config_register_();
  this->update_alert_registers_();

  if (this->shutdown_switch_ != nullptr)
    this->shutdown_switch_->publish_state(state);

  ESP_LOGI(TAG, "INA228 shutdown: %s", state ? "ON (running)" : "OFF (power-down)");
}

// ===========================================================================
// set_alert_function()
// ===========================================================================
void INA228Component::set_alert_function(const std::string &function) {
  this->alert_function_ = function;
  this->update_alert_registers_();

  if (this->alert_select_ != nullptr)
    this->alert_select_->publish_state(function);

  ESP_LOGI(TAG, "INA228 alert function: %s", function.c_str());
}

// ===========================================================================
// set_alert_limit()
// ===========================================================================
void INA228Component::set_alert_limit(float limit) {
  this->alert_limit_ = limit;
  this->update_alert_registers_();

  if (this->alert_limit_number_ != nullptr)
    this->alert_limit_number_->publish_state(limit);

  ESP_LOGI(TAG, "INA228 alert limit: %.4f", limit);
}

// ===========================================================================
// write_config_register_() — CONFIG (0x00)
// ===========================================================================
void INA228Component::write_config_register_() {
  ConfigRegister cfg;
  cfg.raw      = 0;
  cfg.adcrange = this->adc_range_ ? 1 : 0;

  uint16_t swapped = be16(cfg.raw);

  if (!this->write_byte_16(INA228_REG_CONFIG, swapped)) {
    ESP_LOGE(TAG, "Chyba zápisu CONFIG registra");
    this->status_set_warning();
  }
}



// ===========================================================================
// write_adc_config_register_() — ADC_CONFIG (0x01)
// MODE: 000 = Power-Down, 111 = Shunt+Bus+Temp Continuous
// ===========================================================================


void INA228Component::write_adc_config_register_() {
  AdcConfigRegister adc_cfg;
  adc_cfg.raw    = 0;
  //adc_cfg.mode   = this->shutdown_ ? 0 : 15; // 15 je Continuous mode
  adc_cfg.mode   = this->shutdown_ ? 0x0 : 0xF; // 15 je Continuous mode
  adc_cfg.vbusct = (uint16_t) this->adc_time_voltage_;
  adc_cfg.vshct  = (uint16_t) this->adc_time_current_;
  adc_cfg.vtct   = (uint16_t) this->adc_time_temp_;
  adc_cfg.avg    = (uint16_t) this->adc_avg_samples_;

  uint16_t swapped = be16(adc_cfg.raw);


  if (!this->write_byte_16(INA228_REG_ADC_CONFIG, swapped)) {
    ESP_LOGE(TAG, "Chyba zápisu ADC_CONFIG");
    this->status_set_warning();
  }
}


// ===========================================================================
// update_alert_registers_()
// DIAG_ALRT (0x0B) + príslušný limit register (0x0C–0x11)
// ===========================================================================
void INA228Component::update_alert_registers_() {
  if (this->shutdown_ || this->alert_function_ == ALERT_OPT_NONE) {
    this->write_byte_16(INA228_REG_DIAG_ALRT, 0x0000);
    this->write_byte_16(INA228_REG_SOVL, 0x0000);
    this->write_byte_16(INA228_REG_SUVL, 0x0000);
    this->write_byte_16(INA228_REG_BOVL, 0x0000);
    this->write_byte_16(INA228_REG_BUVL, 0x0000);
    this->write_byte_16(INA228_REG_PWR_LIMIT, 0x0000);
    ESP_LOGD(TAG, "Alert: disabled");
    return;
  }

  uint16_t diag_mask = this->alert_function_to_diag_bit_(this->alert_function_);
  if (!this->write_byte_16(INA228_REG_DIAG_ALRT, be16(diag_mask))) {
    ESP_LOGE(TAG, "Chyba zápisu DIAG_ALRT registra");
    this->status_set_warning();
    return;
  }

  uint16_t raw_limit = this->alert_limit_to_raw_(this->alert_limit_);

  // Vyber správny limit register podľa funkcie
  uint8_t limit_reg = 0;
  if (this->alert_function_ == ALERT_OPT_SHNTOL) limit_reg = INA228_REG_SOVL;
  else if (this->alert_function_ == ALERT_OPT_SHNTUL) limit_reg = INA228_REG_SUVL;
  else if (this->alert_function_ == ALERT_OPT_BUSOL)  limit_reg = INA228_REG_BOVL;
  else if (this->alert_function_ == ALERT_OPT_BUSUL)  limit_reg = INA228_REG_BUVL;
  else if (this->alert_function_ == ALERT_OPT_POL)    limit_reg = INA228_REG_PWR_LIMIT;

  if (limit_reg != 0) {
    if (!this->write_byte_16(limit_reg, be16(raw_limit))) {
      ESP_LOGE(TAG, "Chyba zápisu alert limit registra");
      this->status_set_warning();
      return;
    }
  }

  ESP_LOGD(TAG, "Alert: diag=0x%04X, limit_reg=0x%02X, raw=0x%04X (%.4f)",
           diag_mask, limit_reg, raw_limit, this->alert_limit_);
}

// ===========================================================================
// alert_function_to_diag_bit_()
// Vráti bitovú masku pre DIAG_ALRT register
// ===========================================================================
uint16_t INA228Component::alert_function_to_diag_bit_(const std::string &function) {
  if (function == ALERT_OPT_SHNTOL) return static_cast<uint16_t>(ALERT_SHNTOL);
  if (function == ALERT_OPT_SHNTUL) return static_cast<uint16_t>(ALERT_SHNTUL);
  if (function == ALERT_OPT_BUSOL)  return static_cast<uint16_t>(ALERT_BUSOL);
  if (function == ALERT_OPT_BUSUL)  return static_cast<uint16_t>(ALERT_BUSUL);
  if (function == ALERT_OPT_POL)    return static_cast<uint16_t>(ALERT_POL);
  return 0x0000;
}

// ===========================================================================
// alert_limit_to_raw_()
// Konverzia fyzikálnej hodnoty na raw register
//
// SOVL/SUVL: Shunt Voltage [V]
//   ADCRANGE=0: LSB = 312.5 nV  → raw = limit / 312.5e-9
//   ADCRANGE=1: LSB = 78.125 nV → raw = limit / 78.125e-9
// BOVL/BUVL: Bus Voltage [V]
//   LSB = 3.125 mV → raw = limit / 3.125e-3
// PWR_LIMIT: Power [W]
//   Power_LSB = 3.2 × current_lsb_ → raw = limit / (3.2 * current_lsb_)
// ===========================================================================
uint16_t INA228Component::alert_limit_to_raw_(float limit) {
  if (this->alert_function_ == ALERT_OPT_SHNTOL ||
      this->alert_function_ == ALERT_OPT_SHNTUL) {
    float lsb = this->adc_range_ ? 78.125e-9f : 312.5e-9f;
    return static_cast<uint16_t>(limit / lsb);

  } else if (this->alert_function_ == ALERT_OPT_BUSOL ||
             this->alert_function_ == ALERT_OPT_BUSUL) {
    return static_cast<uint16_t>(limit / 3.125e-3f);

  } else if (this->alert_function_ == ALERT_OPT_POL) {
    float power_lsb = 3.2f * this->current_lsb_;
    return static_cast<uint16_t>(limit / power_lsb);
  }

  return 0x0000;
}

// ===========================================================================
// read_register_24_()
// Prečíta 24-bitový register cez I2C (3 bajty, MSB first)
// ===========================================================================
bool INA228Component::read_register_24_(uint8_t reg, uint32_t *out) {
  uint8_t buf[3];
  if (!this->read_bytes(reg, buf, 3))
    return false;
  *out = (static_cast<uint32_t>(buf[0]) << 16) |
         (static_cast<uint32_t>(buf[1]) << 8)  |
          static_cast<uint32_t>(buf[2]);
  return true;
}

// ===========================================================================
// twos_complement_20_()
// Interpretuje 20-bitovú hodnotu ako signed (dvojkový doplnok)
// ===========================================================================
int32_t INA228Component::twos_complement_20_(int32_t val) {
  if (val & (1 << 19))
    val -= (1 << 20);
  return val;
}

// ===========================================================================
// INA228ShutdownSwitch::write_state()
// ===========================================================================
void INA228ShutdownSwitch::write_state(bool state) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_shutdown(state);
}

// ===========================================================================
// INA228AlertSelect::control()
// ===========================================================================
void INA228AlertSelect::control(const std::string &value) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_alert_function(value);
}

// ===========================================================================
// INA228AlertNumber::control()
// ===========================================================================
void INA228AlertNumber::control(float value) {
  if (this->parent_ == nullptr)
    return;
  this->parent_->set_alert_limit(value);
}

}  // namespace ina228
}  // namespace esphome
