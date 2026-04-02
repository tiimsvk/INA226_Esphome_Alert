import esphome.codegen as cg
from esphome.components import i2c, sensor, switch, select, number
import esphome.config_validation as cv
from esphome.const import (
    CONF_BUS_VOLTAGE,
    CONF_CURRENT,
    CONF_ID,
    CONF_MAX_CURRENT,
    CONF_POWER,
    CONF_SHUNT_RESISTANCE,
    CONF_SHUNT_VOLTAGE,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_VOLT,
    UNIT_WATT,
)

DEPENDENCIES = ["i2c"]

# ¶¶ KonfiguraËnÈ kæ˙Ëe ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
CONF_ADC_AVERAGING   = "adc_averaging"
CONF_ADC_TIME        = "adc_time"
CONF_ADC_TIME_TEMP   = "adc_time_temp"
CONF_ADC_RANGE       = "adc_range"
CONF_SHUTDOWN_MODE   = "shutdown_mode"
CONF_ALERT_FUNCTION  = "alert_function"
CONF_ALERT_LIMIT     = "alert_limit"
CONF_TEMPERATURE     = "temperature"

# ¶¶ Namespace a triedy ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
ina228_ns = cg.esphome_ns.namespace("ina228")

INA228Component      = ina228_ns.class_("INA228Component", cg.PollingComponent, i2c.I2CDevice)
INA228ShutdownSwitch = ina228_ns.class_("INA228ShutdownSwitch", switch.Switch)
INA228AlertSelect    = ina228_ns.class_("INA228AlertSelect",    select.Select)
INA228AlertNumber    = ina228_ns.class_("INA228AlertNumber",    number.Number)

# ¶¶ Enumer·cie ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
AdcTime = ina228_ns.enum("AdcTime")
ADC_TIMES = {
    50:   AdcTime.ADC_TIME_50US,
    84:   AdcTime.ADC_TIME_84US,
    150:  AdcTime.ADC_TIME_150US,
    280:  AdcTime.ADC_TIME_280US,
    540:  AdcTime.ADC_TIME_540US,
    1052: AdcTime.ADC_TIME_1052US,
    2074: AdcTime.ADC_TIME_2074US,
    4120: AdcTime.ADC_TIME_4120US,
}

AdcAvgSamples = ina228_ns.enum("AdcAvgSamples")
ADC_AVG_SAMPLES = {
    1:    AdcAvgSamples.ADC_AVG_SAMPLES_1,
    4:    AdcAvgSamples.ADC_AVG_SAMPLES_4,
    16:   AdcAvgSamples.ADC_AVG_SAMPLES_16,
    64:   AdcAvgSamples.ADC_AVG_SAMPLES_64,
    128:  AdcAvgSamples.ADC_AVG_SAMPLES_128,
    256:  AdcAvgSamples.ADC_AVG_SAMPLES_256,
    512:  AdcAvgSamples.ADC_AVG_SAMPLES_512,
    1024: AdcAvgSamples.ADC_AVG_SAMPLES_1024,
    2048: AdcAvgSamples.ADC_AVG_SAMPLES_2048,
    4096: AdcAvgSamples.ADC_AVG_SAMPLES_4096,
}

# Moûnosti alert funkciÌ
ALERT_FUNCTION_OPTIONS = [
    "None",
    "Shunt Over-Limit",
    "Shunt Under-Limit",
    "Bus Over-Limit",
    "Bus Under-Limit",
    "Power Over-Limit",
]

# ¶¶ Valid·tor ADC Ëasu ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
def validate_adc_time(value):
    value = cv.positive_time_period_microseconds(value).total_microseconds
    return cv.enum(ADC_TIMES, int=True)(value)


# ¶¶ SchÈma konfigur·cie ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(INA228Component),

            # ¶¶ Senzory ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            cv.Optional(CONF_BUS_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SHUNT_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=7,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=4,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # INA228 m· zabudovan˝ teplotn˝ senzor (die temperature)
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),

            # ¶¶ Parametre merania ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            cv.Optional(CONF_SHUNT_RESISTANCE, default=0.1): cv.All(
                cv.resistance, cv.Range(min=0.0)
            ),
            cv.Optional(CONF_MAX_CURRENT, default=3.2): cv.All(
                cv.current, cv.Range(min=0.0)
            ),

            # ADC Ëas pre nap‰tie a pr˙d (spoloËn˝ alebo individu·lny)
            cv.Optional(CONF_ADC_TIME, default="1052 us"): cv.Any(
                validate_adc_time,
                cv.Schema(
                    {
                        cv.Required(CONF_VOLTAGE): validate_adc_time,
                        cv.Required(CONF_CURRENT): validate_adc_time,
                    }
                ),
            ),
            # ADC Ëas pre teplotu (samostatn˝)
            cv.Optional(CONF_ADC_TIME_TEMP, default="1052 us"): validate_adc_time,

            cv.Optional(CONF_ADC_AVERAGING, default=4): cv.enum(
                ADC_AVG_SAMPLES, int=True
            ),

            # ADCRANGE: false = ±163.84 mV (vyööÌ rozsah, niûöia citlivosù)
            #           true  = ±40.96 mV  (niûöÌ rozsah, vyööia citlivosù)
            cv.Optional(CONF_ADC_RANGE, default=False): cv.boolean,

            # ¶¶ Ovl·dacie entity ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            cv.Optional(CONF_SHUTDOWN_MODE): switch.switch_schema(
                INA228ShutdownSwitch,
            ),
            cv.Optional(CONF_ALERT_FUNCTION): select.select_schema(
                INA228AlertSelect,
            ),
            cv.Optional(CONF_ALERT_LIMIT): number.number_schema(
                INA228AlertNumber,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x40))
)


# ¶¶ Generovanie C++ kÛdu ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Z·kladnÈ parametre
    cg.add(var.set_shunt_resistance_ohm(config[CONF_SHUNT_RESISTANCE]))
    cg.add(var.set_max_current_a(config[CONF_MAX_CURRENT]))
    cg.add(var.set_adc_range(config[CONF_ADC_RANGE]))

    # ADC Ëasy
    adc_time_config = config[CONF_ADC_TIME]
    if isinstance(adc_time_config, dict):
        cg.add(var.set_adc_time_voltage(adc_time_config[CONF_VOLTAGE]))
        cg.add(var.set_adc_time_current(adc_time_config[CONF_CURRENT]))
    else:
        cg.add(var.set_adc_time_voltage(adc_time_config))
        cg.add(var.set_adc_time_current(adc_time_config))

    cg.add(var.set_adc_time_temp(config[CONF_ADC_TIME_TEMP]))
    cg.add(var.set_adc_avg_samples(config[CONF_ADC_AVERAGING]))

    # Senzory
    if CONF_BUS_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BUS_VOLTAGE])
        cg.add(var.set_bus_voltage_sensor(sens))

    if CONF_SHUNT_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_SHUNT_VOLTAGE])
        cg.add(var.set_shunt_voltage_sensor(sens))

    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))

    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))

    # ¶¶ Shutdown switch ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    if CONF_SHUTDOWN_MODE in config:
        sw = await switch.new_switch(config[CONF_SHUTDOWN_MODE])
        cg.add(sw.set_parent(var))
        cg.add(var.set_shutdown_switch(sw))

    # ¶¶ Alert function select ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    if CONF_ALERT_FUNCTION in config:
        sel = await select.new_select(
            config[CONF_ALERT_FUNCTION],
            options=ALERT_FUNCTION_OPTIONS,
        )
        cg.add(sel.set_parent(var))
        cg.add(var.set_alert_select(sel))

    # ¶¶ Alert limit number ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    if CONF_ALERT_LIMIT in config:
        num = await number.new_number(
            config[CONF_ALERT_LIMIT],
            min_value=0.0,
            max_value=1000.0,
            step=0.000001,   # 1 µV / 1 µA presnosù
        )
        cg.add(num.set_parent(var))
        cg.add(var.set_alert_limit_number(num))
