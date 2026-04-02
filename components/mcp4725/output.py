import esphome.codegen as cg
from esphome.components import i2c, output
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

mcp4725 = cg.esphome_ns.namespace("mcp4725")
MCP4725 = mcp4725.class_("MCP4725", output.FloatOutput, cg.Component, i2c.I2CDevice)

CONF_SAVE_TO_EEPROM = "save_to_eeprom"

CONFIG_SCHEMA = (
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(MCP4725),
            cv.Optional(CONF_SAVE_TO_EEPROM, default=False): cv.boolean,
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

    if config.get(CONF_SAVE_TO_EEPROM):
        cg.add(var.set_save_to_eeprom(config[CONF_SAVE_TO_EEPROM]))