# SI1145 Sensor (ESPHome External Component)

SI1145 UV/IR/Visible sensor component for ESPHome.

## Features

- Visible channel (offset-corrected, range/gain adjusted)
- Infrared channel (offset-corrected, range/gain adjusted)
- UV index
- Calculated lux (`calculated_lux`)
- Extra raw channels for debugging:
	- `visible_raw`
	- `infrared_raw`
- Optional per-channel tuning for visible/infrared:
	- `mode`: `auto` or `manual`
	- `gain`: configurable per channel
	- `range`: `high` or `low`
	- `temp_correction`: `true` or `false`

## YAML (Full Example)

```yaml
external_components:
	- source:
			type: git
			url: https://github.com/dichterDichterD/esp-components
		components: [ si1145_sensor ]

i2c:
	sda: GPIOXX # add your SDA
	scl: GPIOXX # add your SCL
	id: bus_a # optional
	frequency: 100khz # IMPORTANT: required, otherwise values may stay at 0

sensor:
	- platform: si1145_sensor
		id: uv_chip
		address: 0x60
		update_interval: 10s

		visible:
			name: "SI1145 Visible"
			mode: auto
			gain: 0
			range: high
			temp_correction: false

		infrared:
			name: "SI1145 Infrared"
			mode: auto
			gain: 0
			range: high
			temp_correction: false

		uv_index:
			name: "SI1145 UV Index"

		calculated_lux:
			name: "SI1145 Lux"
			unit_of_measurement: lx
			device_class: illuminance
			state_class: measurement

		visible_raw:
			name: "SI1145 Visible Raw"

		infrared_raw:
			name: "SI1145 Infrared Raw"
```

## Available Keys

- `address` (optional, default: `0x60`)
- `update_interval` (optional, default: `60s`)
- `visible` (optional)
	- `mode` (optional, default: `auto`, values: `auto`/`manual`)
	- `gain` (optional, default: `0`, range: `0..7`)
	- `range` (optional, default: `high`, values: `high`/`low`)
	- `temp_correction` (optional, default: `false`)
- `infrared` (optional)
	- `mode` (optional, default: `auto`, values: `auto`/`manual`)
	- `gain` (optional, default: `0`, range: `0..3`)
	- `range` (optional, default: `high`, values: `high`/`low`)
	- `temp_correction` (optional, default: `false`)
- `uv_index` (optional)
- `calculated_lux` (optional)
- `visible_raw` (optional)
- `infrared_raw` (optional)

## Notes

- Use `frequency: 100khz` on I2C to avoid zero readings.
- `visible` and `infrared` are offset-corrected and scaled by selected gain/range.
- `visible_raw` and `infrared_raw` are direct register values for debugging.
- `calculated_lux` is derived from visible and infrared channels.
