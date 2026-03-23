#include "ip5310.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ip5310 {

static const char *TAG = "ip5310";

void IP5310::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IP5310...");
}

void IP5310::update() {
  uint8_t val;

  // 1. Stav batérie v %
  if (this->read_register(IP5310_REG_SOC_DATA, &val, 1) == i2c::ERROR_OK) {
    if (this->battery_level_ != nullptr) this->battery_level_->publish_state(val);
  }

  // 2. Napätie batérie (12-bit ADC)
  uint8_t vbat_l, vbat_h;
  if (this->read_register(IP5310_REG_BAT_L, &vbat_l, 1) == i2c::ERROR_OK &&
      this->read_register(IP5310_REG_BAT_H, &vbat_h, 1) == i2c::ERROR_OK) {
    uint16_t adc = (uint16_t(vbat_h & 0x0F) << 8) | vbat_l;
    // Formula z datasheetu: V = ADC * 1.357mV + 2.333V (približne)
    float voltage = (adc * 0.001357f) + 2.333f;
    if (this->battery_voltage_ != nullptr) this->battery_voltage_->publish_state(voltage);
  }

  // 3. NTC Napätie
  uint8_t ntc_l, ntc_h;
  if (this->read_register(IP5310_REG_NTC_L, &ntc_l, 1) == i2c::ERROR_OK &&
      this->read_register(IP5310_REG_NTC_H, &ntc_h, 1) == i2c::ERROR_OK) {
    uint16_t adc = (uint16_t(ntc_h & 0x0F) << 8) | ntc_l;
    float ntc_v = adc * 0.001073f; // Formula: ADC * 1.073mV
    if (this->ntc_voltage_ != nullptr) this->ntc_voltage_->publish_state(ntc_v);
  }

  // 4. Binárne senzory (VBUS, Full Charge)
  if (this->read_register(IP5310_REG_READ0, &val, 1) == i2c::ERROR_OK) {
    if (this->charger_connected_ != nullptr) this->charger_connected_->publish_state(val & 0x08); // VBUS_VLD
    if (this->charge_full_ != nullptr) this->charge_full_->publish_state(val & 0x02); // CHG_FULL
  }

  // 5. Poruchy (Thermal, OVP)
  if (this->read_register(IP5310_REG_READ2, &val, 1) == i2c::ERROR_OK) {
    if (this->thermal_shutdown_ != nullptr) this->thermal_shutdown_->publish_state(val & 0x0C); // IC + BAT Thermal
    if (this->ovp_error_ != nullptr) this->ovp_error_->publish_state(val & 0x60); // VBUS + BAT OVP
  }
}

void IP5310::write_register_bit(uint8_t reg, uint8_t mask, bool value) {
  uint8_t reg_val;
  if (this->read_register(reg, &reg_val, 1) != i2c::ERROR_OK) return;
  if (value) reg_val |= mask; else reg_val &= ~mask;
  this->write_register(reg, &reg_val, 1);
}

void IP5310Switch::write_state(bool state) {
  if (type == BOOST_EN) parent->write_register_bit(IP5310_REG_SYS_CTL1, 0x02, state);
  if (type == CHARGER_EN) parent->write_register_bit(IP5310_REG_SYS_CTL1, 0x01, state);
  this->publish_state(state);
}

void IP5310Select::control(const std::string &value) {
  if (type == CHARGE_CURRENT) {
    // 50mA step. Ak chceš 1000mA -> 1000/50 = 20. Register 0x21: bity 4:0.
    int ma = atoi(value.c_str());
    uint8_t raw = (ma / 50) - 1;
    if (raw > 31) raw = 31;
    parent->write_register(IP5310_REG_CHG_CTL2, &raw, 1);
  } else if (type == TERM_CURRENT) {
      uint8_t raw = 0;
      if (value == "200mA") raw = 0; else if (value == "400mA") raw = 1; 
      else if (value == "500mA") raw = 2; else if (value == "600mA") raw = 3;
      parent->write_register(IP5310_REG_CHG_CTL4, &raw, 1);
  }
  this->publish_state(value);
}

void IP5310::dump_config() { ESP_LOGCONFIG(TAG, "IP5310 Power Management"); }

// prida niekde medzi implementácie (napr. pod write_register_bit)
void IP5310::write_register_bits(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t value) {
  uint8_t reg_val;
  if (this->read_register(reg, &reg_val, 1) != i2c::ERROR_OK) return;
  reg_val &= ~mask;
  reg_val |= ( (value << shift) & mask );
  this->write_register(reg, &reg_val, 1);
}

} // namespace ip5310
} // namespace esphome