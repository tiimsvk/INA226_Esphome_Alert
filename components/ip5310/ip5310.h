#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace ip5310 {

// Registre pod¾a PDF IP5310-REG-V1-2
static const uint8_t IP5310_REG_SYS_CTL1 = 0x01; // Boost/Charger Enable
static const uint8_t IP5310_REG_CHG_CTL2 = 0x21; // Charge Current (50mA step)
static const uint8_t IP5310_REG_CHG_CTL4 = 0x23; // Termination Current
static const uint8_t IP5310_REG_SOC_DATA = 0x2B; // Battery Percent (0-100)
static const uint8_t IP5310_REG_READ0    = 0x2D; // VBUS_VLD, CHG_FULL
static const uint8_t IP5310_REG_READ2    = 0x2F; // OVP, Thermal Shutdown
static const uint8_t IP5310_REG_BAT_L    = 0x30; // Bat Voltage Low
static const uint8_t IP5310_REG_BAT_H    = 0x31; // Bat Voltage High
static const uint8_t IP5310_REG_NTC_L    = 0x32; // NTC Voltage Low
static const uint8_t IP5310_REG_NTC_H    = 0x33; // NTC Voltage High

class IP5310 : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_battery_level(sensor::Sensor *s) { battery_level_ = s; }
  void set_battery_voltage(sensor::Sensor *s) { battery_voltage_ = s; }
  void set_ntc_voltage(sensor::Sensor *s) { ntc_voltage_ = s; }
  
  void set_charger_connected(binary_sensor::BinarySensor *s) { charger_connected_ = s; }
  void set_charge_full(binary_sensor::BinarySensor *s) { charge_full_ = s; }
  void set_thermal_shutdown(binary_sensor::BinarySensor *s) { thermal_shutdown_ = s; }
  void set_ovp_error(binary_sensor::BinarySensor *s) { ovp_error_ = s; }

  void write_register_bit(uint8_t reg, uint8_t mask, bool value);
  void write_register_bits(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t value);

 protected:
  sensor::Sensor *battery_level_{nullptr};
  sensor::Sensor *battery_voltage_{nullptr};
  sensor::Sensor *ntc_voltage_{nullptr};
  binary_sensor::BinarySensor *charger_connected_{nullptr};
  binary_sensor::BinarySensor *charge_full_{nullptr};
  binary_sensor::BinarySensor *thermal_shutdown_{nullptr};
  binary_sensor::BinarySensor *ovp_error_{nullptr};
};

// Pomocné triedy pre Switch a Select (budú implementované v .cpp)
class IP5310Switch : public switch_::Switch, public Component {
 public:
  enum Type { BOOST_EN, CHARGER_EN } type;
  IP5310 *parent;
  void write_state(bool state) override;
};

class IP5310Select : public select::Select, public Component {
 public:
  enum Type { CHARGE_CURRENT, TERM_CURRENT } type;
  IP5310 *parent;
  void control(const std::string &value) override;
};

}  // namespace ip5310
}  // namespace esphome