#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace ina228 {

// ---------------------------------------------------------------------------
// ADC conversion time (platí pre VBUSCT, VSHCT, VTCT v ADC_CONFIG registri)
// ---------------------------------------------------------------------------
enum AdcTime : uint16_t {
  ADC_TIME_50US   = 0,
  ADC_TIME_84US   = 1,
  ADC_TIME_150US  = 2,
  ADC_TIME_280US  = 3,
  ADC_TIME_540US  = 4,
  ADC_TIME_1052US = 5,
  ADC_TIME_2074US = 6,
  ADC_TIME_4120US = 7
};

// ---------------------------------------------------------------------------
// Počet spriemerovaných vzoriek (AVG, 4 bity v ADC_CONFIG)
// ---------------------------------------------------------------------------
enum AdcAvgSamples : uint16_t {
  ADC_AVG_SAMPLES_1    = 0,
  ADC_AVG_SAMPLES_4    = 1,
  ADC_AVG_SAMPLES_16   = 2,
  ADC_AVG_SAMPLES_64   = 3,
  ADC_AVG_SAMPLES_128  = 4,
  ADC_AVG_SAMPLES_256  = 5,
  ADC_AVG_SAMPLES_512  = 6,
  ADC_AVG_SAMPLES_1024 = 7,
  ADC_AVG_SAMPLES_2048 = 8,
  ADC_AVG_SAMPLES_4096 = 9,
  ADC_AVG_SAMPLES_8192 = 10,  // 10–15 sú aliasy 4096 podľa datasheetu
};

// ---------------------------------------------------------------------------
// ADC_CONFIG register (0x01) — bitové polia
// ---------------------------------------------------------------------------
union AdcConfigRegister {
  uint16_t raw;
  struct {
    uint16_t mode   : 4;  // bits 15..12
    uint16_t vbusct : 3;  // bits 11..9
    uint16_t vshct  : 3;  // bits 8..6
    uint16_t vtct   : 3;  // bits 5..3
    uint16_t avg    : 3;  // bits 2..0
  } __attribute__((packed));
};

// ---------------------------------------------------------------------------
// CONFIG register (0x00) — bitové polia
// ---------------------------------------------------------------------------
union ConfigRegister {
  uint16_t raw;
  struct {
    uint16_t reserved1  : 6;  // bits [5:0]
    uint16_t adcrange   : 1;  // bit  [6]  – 0=±163.84mV, 1=±40.96mV
    uint16_t reserved2  : 1;  // bit  [7]
    uint16_t tempcomp   : 1;  // bit  [8]  – Temp compensation enable
    uint16_t convdly    : 5;  // bits [13:9] – Conversion delay
    uint16_t rstacc     : 1;  // bit  [14] – Reset accumulators
    uint16_t rst        : 1;  // bit  [15] – Full reset
  } __attribute__((packed));
};

// ---------------------------------------------------------------------------
// Alert funkcie – zodpovedajú bitom DIAG_ALRT registra (0x0B)
// Bity D6..D2 sú enable bity pre jednotlivé alert funkcie (datasheet Table 7-17)
// ---------------------------------------------------------------------------
enum AlertFunction : uint16_t {
  ALERT_NONE   = 0x0000,
  ALERT_SHNTOL = (1 << 6),  // Shunt Voltage Over-Limit  (bit D6)
  ALERT_SHNTUL = (1 << 5),  // Shunt Voltage Under-Limit (bit D5)
  ALERT_BUSOL  = (1 << 4),  // Bus Voltage Over-Limit    (bit D4)
  ALERT_BUSUL  = (1 << 3),  // Bus Voltage Under-Limit   (bit D3)
  ALERT_POL    = (1 << 2),  // Power Over-Limit          (bit D2)
};

// ---------------------------------------------------------------------------
// Forward deklarácie
// ---------------------------------------------------------------------------
class INA228Component;

// ---------------------------------------------------------------------------
// Shutdown Switch
// ---------------------------------------------------------------------------
class INA228ShutdownSwitch : public switch_::Switch {
 public:
  void set_parent(INA228Component *parent) { parent_ = parent; }
 protected:
  void write_state(bool state) override;
  INA228Component *parent_{nullptr};
};

// ---------------------------------------------------------------------------
// Alert Function Select
// ---------------------------------------------------------------------------
class INA228AlertSelect : public select::Select {
 public:
  void set_parent(INA228Component *parent) { parent_ = parent; }
 protected:
  void control(const std::string &value) override;
  INA228Component *parent_{nullptr};
};

// ---------------------------------------------------------------------------
// Alert Limit Number
// ---------------------------------------------------------------------------
class INA228AlertNumber : public number::Number {
 public:
  void set_parent(INA228Component *parent) { parent_ = parent; }
 protected:
  void control(float value) override;
  INA228Component *parent_{nullptr};
};

// ---------------------------------------------------------------------------
// Hlavný komponent
// ---------------------------------------------------------------------------
class INA228Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  // Základná konfigurácia
  void set_shunt_resistance_ohm(float r)      { shunt_resistance_ohm_ = r; }
  void set_max_current_a(float i)             { max_current_a_ = i; }
  void set_adc_time_voltage(AdcTime t)        { adc_time_voltage_ = t; }
  void set_adc_time_current(AdcTime t)        { adc_time_current_ = t; }
  void set_adc_time_temp(AdcTime t)           { adc_time_temp_ = t; }
  void set_adc_avg_samples(AdcAvgSamples s)   { adc_avg_samples_ = s; }
  void set_adc_range(bool range)              { adc_range_ = range; }

  // Senzory
  void set_bus_voltage_sensor(sensor::Sensor *s)   { bus_voltage_sensor_   = s; }
  void set_shunt_voltage_sensor(sensor::Sensor *s) { shunt_voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s)       { current_sensor_       = s; }
  void set_power_sensor(sensor::Sensor *s)         { power_sensor_         = s; }
  void set_temperature_sensor(sensor::Sensor *s)   { temperature_sensor_   = s; }

  // Ovládacie entity
  void set_shutdown_switch(INA228ShutdownSwitch *sw) { shutdown_switch_    = sw; }
  void set_alert_select(INA228AlertSelect *sel)       { alert_select_       = sel; }
  void set_alert_limit_number(INA228AlertNumber *num) { alert_limit_number_ = num; }

  // Akčné metódy
  void set_shutdown(bool enabled);
  void set_alert_function(const std::string &function);
  void set_alert_limit(float limit);

  void set_temp_compensation(bool enabled) { temp_comp_enabled_ = enabled; }
  void set_shunt_tempco_ppm(float ppm)     { shunt_tempco_ppm_ = ppm; }
  
 protected:
  // Konfiguračné parametre
  float         shunt_resistance_ohm_{0.1f};
  float         max_current_a_{3.2f};
  // current_lsb_ v ampéroch (float, INA228 má 20-bit CURRENT register)
  float         current_lsb_{0.0f};
  // ADCRANGE: false = ±163.84 mV (VSHUNT LSB = 312.5 nV)
  //           true  = ±40.96 mV  (VSHUNT LSB = 78.125 nV)
  bool          adc_range_{false};

  // Teplotná kompenzácia shuntu (TCR) v ppm/°C, typicky napr. 50..200
  bool  temp_comp_enabled_{false};
  float shunt_tempco_ppm_{75.0f};
  
  AdcTime       adc_time_voltage_{AdcTime::ADC_TIME_1052US};
  AdcTime       adc_time_current_{AdcTime::ADC_TIME_1052US};
  AdcTime       adc_time_temp_   {AdcTime::ADC_TIME_1052US};
  AdcAvgSamples adc_avg_samples_ {AdcAvgSamples::ADC_AVG_SAMPLES_4};

  // Senzory
  sensor::Sensor *bus_voltage_sensor_  {nullptr};
  sensor::Sensor *shunt_voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_      {nullptr};
  sensor::Sensor *power_sensor_        {nullptr};
  sensor::Sensor *temperature_sensor_  {nullptr};

  // Ovládacie entity
  INA228ShutdownSwitch *shutdown_switch_    {nullptr};
  INA228AlertSelect    *alert_select_       {nullptr};
  INA228AlertNumber    *alert_limit_number_ {nullptr};

  // Interný stav
  bool        shutdown_      {false};
  std::string alert_function_{"None"};
  float       alert_limit_   {0.0f};

  // Pomocné metódy
  int32_t  twos_complement_20_(int32_t val);
  void     write_config_register_();
  void     write_adc_config_register_();
  void     update_alert_registers_();
  uint16_t alert_function_to_diag_bit_(const std::string &function);
  uint16_t alert_limit_to_raw_(float limit);

  // Čítanie 24-bitového registra cez I2C (vráti raw uint32)
  bool read_register_24_(uint8_t reg, uint32_t *out);
};

}  // namespace ina228
}  // namespace esphome
