#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace ina226 {

enum AdcTime : uint16_t {
  ADC_TIME_140US  = 0,
  ADC_TIME_204US  = 1,
  ADC_TIME_332US  = 2,
  ADC_TIME_588US  = 3,
  ADC_TIME_1100US = 4,
  ADC_TIME_2116US = 5,
  ADC_TIME_4156US = 6,
  ADC_TIME_8244US = 7
};

enum AdcAvgSamples : uint16_t {
  ADC_AVG_SAMPLES_1    = 0,
  ADC_AVG_SAMPLES_4    = 1,
  ADC_AVG_SAMPLES_16   = 2,
  ADC_AVG_SAMPLES_64   = 3,
  ADC_AVG_SAMPLES_128  = 4,
  ADC_AVG_SAMPLES_256  = 5,
  ADC_AVG_SAMPLES_512  = 6,
  ADC_AVG_SAMPLES_1024 = 7
};

// Enum pre Alert funkcie - zodpovedá bitom Mask/Enable registra (06h)
// Bity D15..D11: SOL, SUL, BOL, BUL, POL
enum AlertFunction : uint16_t {
  ALERT_NONE = 0x0000,
  ALERT_SOL  = 0x8000,  // Shunt Voltage Over-Limit  (bit D15)
  ALERT_SUL  = 0x4000,  // Shunt Voltage Under-Limit (bit D14)
  ALERT_BOL  = 0x2000,  // Bus Voltage Over-Limit    (bit D13)
  ALERT_BUL  = 0x1000,  // Bus Voltage Under-Limit   (bit D12)
  ALERT_POL  = 0x0800,  // Power Over-Limit          (bit D11)
};

union ConfigurationRegister {
  uint16_t raw;
  struct {
    uint16_t mode                         : 3;  // bits [2:0]
    AdcTime  shunt_voltage_conversion_time: 3;  // bits [5:3]
    AdcTime  bus_voltage_conversion_time  : 3;  // bits [8:6]
    AdcAvgSamples avg_samples             : 3;  // bits [11:9]
    uint16_t reserved                     : 3;  // bits [14:12]
    uint16_t reset                        : 1;  // bit  [15]
  } __attribute__((packed));
};

// Predné deklarácie
class INA226Component;

// Shutdown Switch
class INA226ShutdownSwitch : public switch_::Switch {
 public:
  void set_parent(INA226Component *parent) { parent_ = parent; }
 protected:
  void write_state(bool state) override;
  INA226Component *parent_{nullptr};
};

// Alert Function Select
class INA226AlertSelect : public select::Select {
 public:
  void set_parent(INA226Component *parent) { parent_ = parent; }
 protected:
  void control(const std::string &value) override;
  INA226Component *parent_{nullptr};
};

// Alert Limit Number
class INA226AlertNumber : public number::Number {
 public:
  void set_parent(INA226Component *parent) { parent_ = parent; }
 protected:
  void control(float value) override;
  INA226Component *parent_{nullptr};
};

// Hlavný komponent
class INA226Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  // Settery pre základnú konfiguráciu
  void set_shunt_resistance_ohm(float shunt_resistance_ohm) {
    shunt_resistance_ohm_ = shunt_resistance_ohm;
  }
  void set_max_current_a(float max_current_a) { max_current_a_ = max_current_a; }
  void set_adc_time_voltage(AdcTime time)     { adc_time_voltage_ = time; }
  void set_adc_time_current(AdcTime time)     { adc_time_current_ = time; }
  void set_adc_avg_samples(AdcAvgSamples samples) { adc_avg_samples_ = samples; }

  // Settery pre senzory
  void set_bus_voltage_sensor(sensor::Sensor *s)   { bus_voltage_sensor_   = s; }
  void set_shunt_voltage_sensor(sensor::Sensor *s) { shunt_voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s)       { current_sensor_       = s; }
  void set_power_sensor(sensor::Sensor *s)         { power_sensor_         = s; }

  // Settery pre nové entity
  void set_shutdown_switch(INA226ShutdownSwitch *sw)  { shutdown_switch_      = sw;  }
  void set_alert_select(INA226AlertSelect *sel)        { alert_select_         = sel; }
  void set_alert_limit_number(INA226AlertNumber *num)  { alert_limit_number_   = num; }

  // Akčné metódy volané z entít
  // Poznámka: parameter 'enabled' interpretuje true = device enabled (switch ON),
  // false = device in power-down (switch OFF).
  void set_shutdown(bool enabled);
  void set_alert_function(const std::string &function);
  void set_alert_limit(float limit);

 protected:
  // Parametre merania
  float shunt_resistance_ohm_{0.1f};
  float max_current_a_{3.2f};
  // LSB prúdu v mikroampéroch (uložené ako celé číslo pre presnosť)
  uint32_t calibration_lsb_{0};

  AdcTime       adc_time_voltage_ {AdcTime::ADC_TIME_1100US};
  AdcTime       adc_time_current_ {AdcTime::ADC_TIME_1100US};
  AdcAvgSamples adc_avg_samples_  {AdcAvgSamples::ADC_AVG_SAMPLES_4};

  // Senzory
  sensor::Sensor *bus_voltage_sensor_  {nullptr};
  sensor::Sensor *shunt_voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_      {nullptr};
  sensor::Sensor *power_sensor_        {nullptr};

  // Nové entity
  INA226ShutdownSwitch *shutdown_switch_    {nullptr};
  INA226AlertSelect    *alert_select_       {nullptr};
  INA226AlertNumber    *alert_limit_number_ {nullptr};

  // Interný stav
  // shutdown_ == true  => zariadenie je v power-down
  // shutdown_ == false => zariadenie beží
  bool        shutdown_       {false};
  std::string alert_function_ {"None"};
  float       alert_limit_    {0.0f};

  // Pomocné metódy
  int32_t twos_complement_(int32_t val, uint8_t bits);
  void    update_config_register_();
  void    update_alert_registers_();

  // Konverzia názvu funkcie na raw hodnotu Mask/Enable registra
  uint16_t alert_function_to_mask_(const std::string &function);
  // Konverzia fyzikálnej hodnoty na raw hodnotu Alert Limit registra
  uint16_t alert_limit_to_raw_(float limit);
};

}  // namespace ina226
}  // namespace esphome
