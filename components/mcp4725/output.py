import esphome.codegen as cg
from esphome.components import i2c, output
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

mcp4725 = cg.esphome_ns.namespace("mcp4725")
MCP4725 = mcp4725.class_("MCP4725", output.FloatOutput, cg.Component, i2c.I2CDevice)

CONF_SAVE_TO_EEPROM = "save_to_eeprom"
CONF_SAVE_THRESHOLD = "save_threshold"
CONF_SAVE_DEBOUNCE_MS = "save_debounce_ms"
CONF_SAVE_MIN_INTERVAL_S = "save_min_interval_s"

CONFIG_SCHEMA = (
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(MCP4725),
            cv.Optional(CONF_SAVE_TO_EEPROM, default=False): cv.boolean,
            cv.Optional(CONF_SAVE_THRESHOLD, default=0.01): cv.percentage,  # relative 0..1
            cv.Optional(CONF_SAVE_DEBOUNCE_MS, default=3000): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_SAVE_MIN_INTERVAL_S, default=60): cv.positive_time_period_seconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x60))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await output.register_output(var, config)

    cg.add(var.set_save_to_eeprom(config[CONF_SAVE_TO_EEPROM]))
    cg.add(var.set_save_threshold(config[CONF_SAVE_THRESHOLD]))
    cg.add(var.set_save_debounce_ms(config[CONF_SAVE_DEBOUNCE_MS]))
    cg.add(var.set_save_min_interval_s(config[CONF_SAVE_MIN_INTERVAL_S]))
