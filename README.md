INA226 esphome :
Add funcions:
- alert pin select:
  
 • Shunt Voltage Over-Limit (SOL)
  
 • Shunt Voltage Under-Limit (SUL)
 
 • Bus Voltage Over-Limit (BOL)
 
 • Bus Voltage Under-Limit (BUL)
 
 • Power Over-Limit (POL)
 
- alert pin number
- shutdown switch


```yaml
sensor:
  - platform: ina226
    address: 0x40
    shunt_resistance: 0.1 ohm
    max_current: 3.2 A
    update_interval: 5s
    adc_time: 1100 us
    adc_averaging: 16

    bus_voltage:
      name: "INA226 Bus Voltage"
    shunt_voltage:
      name: "INA226 Shunt Voltage"
    current:
      name: "INA226 Current"
    power:
      name: "INA226 Power"

    shutdown_mode:
      name: "INA226 Shutdown"

    alert_function:
      name: "INA226 Alert Function"

    alert_limit:
      name: "INA226 Alert Limit"
      unit_of_measurement: "V"
