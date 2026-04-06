# SI1145 New (ESPHome External Component)

SI1145 UV/IR/Visible sensor component for ESPHome.

## Features

- Visible channel (offset-corrected raw counts)
- Infrared channel (offset-corrected raw counts)
- UV index
- Calculated lux (`calculated_lux`)
- Extra raw channels for debugging:
  - `visible_raw`
  - `infrared_raw`

## YAML (Full Example)

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/dichterDichterD/esp-components
    components: [ si1145_new ]

i2c:
  sda: GPIOXX #add your SDA
  scl: GPIOXX #add your SCL
  id: bus_a #optional
  frequency: 100khz #!! IMPORTANT - Does not work without, gets 0/Zero Readings

sensor:
  - platform: si1145_new
    id: uv_chip
    address: 0x60
    update_interval: 10s

    visible:
      name: "SI1145 Visible"
      icon: mdi:brightness-5

    infrared:
      name: "SI1145 Infrared"
      icon: mdi:weather-night

    uv_index:
      name: "SI1145 UV Index"
      icon: mdi:sun-wireless

    calculated_lux:
      name: "SI1145 Lux"
      unit_of_measurement: lx
      device_class: illuminance
      state_class: measurement

    visible_raw:
      name: "SI1145 Visible Raw"
      icon: mdi:counter

    infrared_raw:
      name: "SI1145 Infrared Raw"
      icon: mdi:counter
```

## Available Keys

- `address` (optional, default: `0x60`)
- `update_interval` (optional, default: `10s`)
- `visible` (optional)
- `infrared` (optional)
- `uv_index` (optional)
- `calculated_lux` (optional)
- `visible_raw` (optional)
- `infrared_raw` (optional)

## Notes

- `visible` and `infrared` are offset-corrected counts.
- `visible_raw` and `infrared_raw` are direct register values.
- Lux is calculated from visible and infrared channels.
- Debug logs are emitted in `DEBUG` log level from the C++ component update cycle.
