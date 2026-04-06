import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_INFRARED,
    CONF_VISIBLE,
    ICON_BRIGHTNESS_5,
    STATE_CLASS_MEASUREMENT,
    UNIT_EMPTY,
)

CONF_UV_INDEX = "uv_index"
ICON_UV = "mdi:sun-wireless"

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
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
            ),
            cv.Optional(CONF_INFRARED): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
            ),
            cv.Optional(CONF_UV_INDEX): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_UV,
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
