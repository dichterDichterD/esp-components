# SI1145 (ESPHome External Component)

Original Component based on https://github.com/berfenger/esphome_components/tree/main/components/si1145. Made some minor changes but added 
#include "esphome/core/hal.h" to make delays(); working.

## YAML (Full Example)

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/dichterDichterD/esp-components
    components: [ si1145 ]

i2c:
  sda: GPIOXX #add your SDA
  scl: GPIOXX #add your SCL
  frequency: 100khz #!! IMPORTANT - Does not work without, gets 0/Zero Readings

sensor:
  - platform: si1145_new
    address: 0x60

    visible:
      name: "SI1145 Visible"
      icon: mdi:brightness-5
    infrared:
      name: "SI1145 Infrared"
      icon: mdi:weather-night
    uv_index:
      name: "SI1145 UV Index"
      icon: mdi:sun-wireless
```
