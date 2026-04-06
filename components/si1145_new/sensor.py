import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_CALCULATED_LUX,
    CONF_ID,
    CONF_INFRARED,
    CONF_VISIBLE,
    DEVICE_CLASS_ILLUMINANCE,
    ICON_BRIGHTNESS_5,
    STATE_CLASS_MEASUREMENT,
    UNIT_EMPTY,
    UNIT_LUX,
)

CONF_UV_INDEX = "uv_index"
CONF_VISIBLE_RAW = "visible_raw"
CONF_INFRARED_RAW = "infrared_raw"
ICON_UV = "mdi:sun-wireless"
ICON_INFRARED = "mdi:weather-night"
ICON_RAW = "mdi:counter"

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

si1145_new_ns = cg.esphome_ns.namespace("si1145_new")

SI1145NewComponent = si1145_new_ns.class_(
    "SI1145NewComponent", cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SI1145NewComponent),
            cv.Optional(CONF_VISIBLE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
            ),
            cv.Optional(CONF_INFRARED): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_INFRARED,
            ),
            cv.Optional(CONF_UV_INDEX): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_UV,
            ),
            cv.Optional(CONF_CALCULATED_LUX): sensor.sensor_schema(
                unit_of_measurement=UNIT_LUX,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
            ),
            cv.Optional(CONF_VISIBLE_RAW): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_RAW,
            ),
            cv.Optional(CONF_INFRARED_RAW): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_RAW,
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(i2c.i2c_device_schema(0x60))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_VISIBLE in config:
        sens = await sensor.new_sensor(config[CONF_VISIBLE])
        cg.add(var.set_visible_sensor(sens))

    if CONF_INFRARED in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED])
        cg.add(var.set_infrared_sensor(sens))

    if CONF_UV_INDEX in config:
        sens = await sensor.new_sensor(config[CONF_UV_INDEX])
        cg.add(var.set_uvindex_sensor(sens))

    if CONF_CALCULATED_LUX in config:
        sens = await sensor.new_sensor(config[CONF_CALCULATED_LUX])
        cg.add(var.set_illuminance_sensor(sens))

    if CONF_VISIBLE_RAW in config:
        sens = await sensor.new_sensor(config[CONF_VISIBLE_RAW])
        cg.add(var.set_visible_raw_sensor(sens))

    if CONF_INFRARED_RAW in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED_RAW])
        cg.add(var.set_infrared_raw_sensor(sens))
