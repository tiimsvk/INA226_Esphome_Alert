
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, i2c, sensor, switch, select
from esphome.const import (
    CONF_ID, 
    UNIT_PERCENT, 
    DEVICE_CLASS_BATTERY, 
    DEVICE_CLASS_VOLTAGE, 
    UNIT_VOLT,
    CONF_UPDATE_INTERVAL,
    CONF_BATTERY_LEVEL,
    CONF_ADDRESS,
)

# Definícia menného priestoru a tried
ip5310_ns = cg.esphome_ns.namespace("ip5310")
IP5310 = ip5310_ns.class_("IP5310", cg.PollingComponent, i2c.I2CDevice)
IP5310Switch = ip5310_ns.class_("IP5310Switch", switch.Switch, cg.Component)
IP5310Select = ip5310_ns.class_("IP5310Select", select.Select, cg.Component)

# Názvy konfiguračných kľúčov
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_NTC_VOLTAGE = "ntc_voltage"
CONF_CHARGER_CONNECTED = "charger_connected"
CONF_CHARGE_FULL = "charge_full"
CONF_THERMAL_SHUTDOWN = "thermal_shutdown"
CONF_OVP_ERROR = "ovp_error"
CONF_BOOST_EN = "boost_en"
CONF_CHARGER_EN = "charger_en"
CONF_CHARGE_CURRENT = "charge_current"

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(IP5310),
        cv.Optional(CONF_ADDRESS, default=0x75): cv.i2c_address,
        
        # Senzory
        cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            device_class=DEVICE_CLASS_BATTERY,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            device_class=DEVICE_CLASS_VOLTAGE,
            accuracy_decimals=2,
        ),
        cv.Optional(CONF_NTC_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
        ),

        # Binárne senzory (Stavy)
        cv.Optional(CONF_CHARGER_CONNECTED): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_CHARGE_FULL): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_THERMAL_SHUTDOWN): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_OVP_ERROR): binary_sensor.binary_sensor_schema(),

        # Prepínače (Switches)
        cv.Optional(CONF_BOOST_EN): switch.switch_schema(IP5310Switch),
        cv.Optional(CONF_CHARGER_EN): switch.switch_schema(IP5310Switch),

        # Výber prúdu (Select)
        cv.Optional(CONF_CHARGE_CURRENT): select.select_schema(IP5310Select),

    })
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x75))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Prepojenie senzorov
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))
    
    if CONF_BATTERY_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_VOLTAGE])
        cg.add(var.set_battery_voltage(sens))

    if CONF_NTC_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_NTC_VOLTAGE])
        cg.add(var.set_ntc_voltage(sens))

    # Prepojenie binárnych senzorov
    if CONF_CHARGER_CONNECTED in config:
        bsens = await binary_sensor.new_binary_sensor(config[CONF_CHARGER_CONNECTED])
        cg.add(var.set_charger_connected(bsens))

    if CONF_CHARGE_FULL in config:
        bsens = await binary_sensor.new_binary_sensor(config[CONF_CHARGE_FULL])
        cg.add(var.set_charge_full(bsens))

    if CONF_THERMAL_SHUTDOWN in config:
        bsens = await binary_sensor.new_binary_sensor(config[CONF_THERMAL_SHUTDOWN])
        cg.add(var.set_thermal_shutdown(bsens))

    if CONF_OVP_ERROR in config:
        bsens = await binary_sensor.new_binary_sensor(config[CONF_OVP_ERROR])
        cg.add(var.set_ovp_error(bsens))

    # Prepojenie prepínačov (Switches)
    if CONF_BOOST_EN in config:
        sw = await switch.new_switch(config[CONF_BOOST_EN])
        cg.add(sw.set_type(0)) # 0 = BOOST_EN v C++ enum
        cg.add(sw.set_parent(var))
        await cg.register_component(sw, config[CONF_BOOST_EN])

    if CONF_CHARGER_EN in config:
        sw = await switch.new_switch(config[CONF_CHARGER_EN])
        cg.add(sw.set_type(1)) # 1 = CHARGER_EN v C++ enum
        cg.add(sw.set_parent(var))
        await cg.register_component(sw, config[CONF_CHARGER_EN])

    # Prepojenie Select (Nabíjací prúd)
    if CONF_CHARGE_CURRENT in config:
        sel = await select.new_select(config[CONF_CHARGE_CURRENT], options=["500", "1000", "1500", "2000", "2500", "3000"])
        cg.add(sel.set_parent(var))
        await cg.register_component(sel, config[CONF_CHARGE_CURRENT])
