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
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_VOLT,
    UNIT_WATT,
)

DEPENDENCIES = ["i2c"]

# ¶¶ KonfiguraËnÈ kæ˙Ëe ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
CONF_ADC_AVERAGING   = "adc_averaging"
CONF_ADC_TIME        = "adc_time"
CONF_SHUTDOWN_MODE   = "shutdown_mode"
CONF_ALERT_FUNCTION  = "alert_function"
CONF_ALERT_LIMIT     = "alert_limit"

# ¶¶ Namespace a triedy ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
ina226_ns = cg.esphome_ns.namespace("ina226")

INA226Component      = ina226_ns.class_("INA226Component", cg.PollingComponent, i2c.I2CDevice)
INA226ShutdownSwitch = ina226_ns.class_("INA226ShutdownSwitch", switch.Switch)
INA226AlertSelect    = ina226_ns.class_("INA226AlertSelect",    select.Select)
INA226AlertNumber    = ina226_ns.class_("INA226AlertNumber",    number.Number)

# ¶¶ Enumer·cie ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
AdcTime = ina226_ns.enum("AdcTime")
ADC_TIMES = {
    140:  AdcTime.ADC_TIME_140US,
    204:  AdcTime.ADC_TIME_204US,
    332:  AdcTime.ADC_TIME_332US,
    588:  AdcTime.ADC_TIME_588US,
    1100: AdcTime.ADC_TIME_1100US,
    2116: AdcTime.ADC_TIME_2116US,
    4156: AdcTime.ADC_TIME_4156US,
    8244: AdcTime.ADC_TIME_8244US,
}

AdcAvgSamples = ina226_ns.enum("AdcAvgSamples")
ADC_AVG_SAMPLES = {
    1:    AdcAvgSamples.ADC_AVG_SAMPLES_1,
    4:    AdcAvgSamples.ADC_AVG_SAMPLES_4,
    16:   AdcAvgSamples.ADC_AVG_SAMPLES_16,
    64:   AdcAvgSamples.ADC_AVG_SAMPLES_64,
    128:  AdcAvgSamples.ADC_AVG_SAMPLES_128,
    256:  AdcAvgSamples.ADC_AVG_SAMPLES_256,
    512:  AdcAvgSamples.ADC_AVG_SAMPLES_512,
    1024: AdcAvgSamples.ADC_AVG_SAMPLES_1024,
}

# Moûnosti alert funkciÌ - musia zodpovedaù konötant·m v .cpp
ALERT_FUNCTION_OPTIONS = [
    "None",
    "Shunt Over-Limit",
    "Shunt Under-Limit",
    "Bus Over-Limit",
    "Bus Under-Limit",
    "Power Over-Limit",
]

# ¶¶ Valid·tor ADC Ëasu ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
def validate_adc_time(value):
    value = cv.positive_time_period_microseconds(value).total_microseconds
    return cv.enum(ADC_TIMES, int=True)(value)


# ¶¶ SchÈma konfigur·cie ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(INA226Component),

            # ¶¶ Senzory ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            cv.Optional(CONF_BUS_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SHUNT_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=5,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),

            # ¶¶ Parametre merania ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            cv.Optional(CONF_SHUNT_RESISTANCE, default=0.1): cv.All(
                cv.resistance, cv.Range(min=0.0)
            ),
            cv.Optional(CONF_MAX_CURRENT, default=3.2): cv.All(
                cv.current, cv.Range(min=0.0)
            ),
            cv.Optional(CONF_ADC_TIME, default="1100 us"): cv.Any(
                validate_adc_time,
                cv.Schema(
                    {
                        cv.Required(CONF_VOLTAGE): validate_adc_time,
                        cv.Required(CONF_CURRENT): validate_adc_time,
                    }
                ),
            ),
            cv.Optional(CONF_ADC_AVERAGING, default=4): cv.enum(
                ADC_AVG_SAMPLES, int=True
            ),

            # ¶¶ Ovl·dacie entity ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
            # Shutdown switch: zapne/vypne power-down mÛd (MODE = 000)
            cv.Optional(CONF_SHUTDOWN_MODE): switch.switch_schema(
                INA226ShutdownSwitch,
            ),

            # Alert function select: vyber ktor˙ veliËinu sledovaù
            cv.Optional(CONF_ALERT_FUNCTION): select.select_schema(
                INA226AlertSelect,
            ),

            # Alert limit number: prahov· hodnota pre alert
            # Rozsah z·visÌ od funkcie, ale d·me rozumnÈ maximum:
            #   - Shunt voltage: max ±81.92 mV õ pouûijeme V, teda 0.08192
            #   - Bus voltage:   max 36 V
            #   - Power:         z·visÌ od aplik·cie, napr. 1000 W
            # D·me jednotn˝ rozsah 0..1000, uûÌvateæ zad·va v SI jednotk·ch:
            #   V pre nap‰tia, W pre v˝kon
            cv.Optional(CONF_ALERT_LIMIT): number.number_schema(
                INA226AlertNumber,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x40))
)


# ¶¶ Generovanie C++ kÛdu ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Z·kladnÈ parametre
    cg.add(var.set_shunt_resistance_ohm(config[CONF_SHUNT_RESISTANCE]))
    cg.add(var.set_max_current_a(config[CONF_MAX_CURRENT]))

    # ADC Ëasy
    adc_time_config = config[CONF_ADC_TIME]
    if isinstance(adc_time_config, dict):
        cg.add(var.set_adc_time_voltage(adc_time_config[CONF_VOLTAGE]))
        cg.add(var.set_adc_time_current(adc_time_config[CONF_CURRENT]))
    else:
        cg.add(var.set_adc_time_voltage(adc_time_config))
        cg.add(var.set_adc_time_current(adc_time_config))

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

    # ¶¶ Shutdown switch ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    if CONF_SHUTDOWN_MODE in config:
        sw = await switch.new_switch(config[CONF_SHUTDOWN_MODE])
        cg.add(sw.set_parent(var))
        cg.add(var.set_shutdown_switch(sw))
        
    # ¶¶ Alert function select ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    # D‘LEéIT…: new_select vyûaduje zoznam options ako druh˝ argument
    if CONF_ALERT_FUNCTION in config:
        sel = await select.new_select(
            config[CONF_ALERT_FUNCTION],
            options=ALERT_FUNCTION_OPTIONS,  # ã povinn˝ parameter!
        )
        cg.add(sel.set_parent(var))
        cg.add(var.set_alert_select(sel))
        
    # ¶¶ Alert limit number ¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶
    if CONF_ALERT_LIMIT in config:
        num = await number.new_number(
            config[CONF_ALERT_LIMIT],
            min_value=0.0,    # Spodn· hranica: 0
            max_value=1000.0, # Horn· hranica: 1000 (W alebo V podæa funkcie)
            step=0.0001,      # Krok: 0.1 mV / 0.1 mA presnosù
        )
        cg.add(num.set_parent(var))
        cg.add(var.set_alert_limit_number(num))
