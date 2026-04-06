#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace si1145_sensor {

static const uint8_t SI1145_PARAM_QUERY = 0x80;
static const uint8_t SI1145_PARAM_SET = 0xA0;
static const uint8_t SI1145_NOP = 0x00;
static const uint8_t SI1145_RESET = 0x01;
static const uint8_t SI1145_PS_FORCE = 0x05;
static const uint8_t SI1145_ALS_FORCE = 0x06;
static const uint8_t SI1145_PSALS_FORCE = 0x07;
static const uint8_t SI1145_PSALS_AUTO = 0x0F;

static const uint8_t SI1145_PARAM_CHLIST = 0x01;
static const uint8_t SI1145_PARAM_CHLIST_ENUV = 0x80;
static const uint8_t SI1145_PARAM_CHLIST_ENALSIR = 0x20;
static const uint8_t SI1145_PARAM_CHLIST_ENALSVIS = 0x10;
static const uint8_t SI1145_PARAM_CHLIST_ENPS1 = 0x01;

static const uint8_t SI1145_PARAM_PSLED12SEL = 0x02;
static const uint8_t SI1145_PARAM_PSLED12SEL_PS1LED1 = 0x01;
static const uint8_t SI1145_REG_PSLED21 = 0x0F;

static const uint8_t SI1145_PARAM_PS1ADCMUX = 0x07;
static const uint8_t SI1145_PARAM_PSADCOUNTER = 0x0A;
static const uint8_t SI1145_PARAM_PSADCGAIN = 0x0B;
static const uint8_t SI1145_PARAM_PSADCMISC = 0x0C;
static const uint8_t SI1145_PARAM_ALSIRADCMUX = 0x0E;
static const uint8_t SI1145_PARAM_ALSVISADCOUNTER = 0x10;
static const uint8_t SI1145_PARAM_ALSVISADCGAIN = 0x11;
static const uint8_t SI1145_PARAM_ALSVISADCMISC = 0x12;
static const uint8_t SI1145_PARAM_ALSIRADCOUNTER = 0x1D;
static const uint8_t SI1145_PARAM_ALSIRADCGAIN = 0x1E;
static const uint8_t SI1145_PARAM_ALSIRADCMISC = 0x1F;

static const uint8_t SI1145_PARAM_PSADCMISC_RANGE = 0x20;
static const uint8_t SI1145_PARAM_PSADCMISC_PSMODE = 0x04;
static const uint8_t SI1145_PARAM_ADCCOUNTER_511CLK = 0x70;
static const uint8_t SI1145_PARAM_ADCMUX_SMALLIR = 0x00;
static const uint8_t SI1145_PARAM_ADCMUX_LARGEIR = 0x03;

static const uint8_t SI1145_REG_PARTID = 0x00;
static const uint8_t SI1145_REG_INTCFG = 0x03;
static const uint8_t SI1145_REG_IRQEN = 0x04;
static const uint8_t SI1145_REG_IRQMODE1 = 0x05;
static const uint8_t SI1145_REG_IRQMODE2 = 0x06;
static const uint8_t SI1145_REG_HWKEY = 0x07;
static const uint8_t SI1145_REG_MEASRATE0 = 0x08;
static const uint8_t SI1145_REG_MEASRATE1 = 0x09;

static const uint8_t SI1145_REG_UCOEFF0 = 0x13;
static const uint8_t SI1145_REG_UCOEFF1 = 0x14;
static const uint8_t SI1145_REG_UCOEFF2 = 0x15;
static const uint8_t SI1145_REG_UCOEFF3 = 0x16;
static const uint8_t SI1145_REG_PARAMWR = 0x17;
static const uint8_t SI1145_REG_COMMAND = 0x18;
static const uint8_t SI1145_REG_RESPONSE = 0x20;
static const uint8_t SI1145_REG_IRQSTAT = 0x21;

static const uint8_t SI1145_REG_ALSVISDATA0 = 0x22;
static const uint8_t SI1145_REG_ALSIRDATA0 = 0x24;
static const uint8_t SI1145_REG_PARAMRD = 0x2E;
static const uint8_t SI1145_REG_UVINDEX0 = 0x2C;
static const uint8_t SI1145_REG_UVINDEX1 = 0x2D;

static const uint8_t SI1145_REG_INTCFG_INTOE = 0x01;
static const uint8_t SI1145_REG_IRQEN_ALSEVERYSAMPLE = 0x01;

static const uint16_t VALUE_AT_ZERO_HIGH = 260;
static const uint16_t VALUE_AT_ZERO_LOW = 270;
static const uint16_t OVERFLOW_VALUE = 0x7FFF;

enum SI1145Range : uint8_t { RANGE_HIGH = 0x20, RANGE_LOW = 0x00 };

class SI1145SensorComponent : public PollingComponent, public i2c::I2CDevice {
 public:
	void set_visible_sensor(sensor::Sensor *visible_sensor) { visible_sensor_ = visible_sensor; }
	void set_infrared_sensor(sensor::Sensor *infrared_sensor) { infrared_sensor_ = infrared_sensor; }
	void set_uvindex_sensor(sensor::Sensor *uvindex_sensor) { uvindex_sensor_ = uvindex_sensor; }
	void set_illuminance_sensor(sensor::Sensor *illuminance_sensor) { illuminance_sensor_ = illuminance_sensor; }

	void set_visible_auto(bool v) { visible_mode_auto_ = v; }
	void set_infrared_auto(bool v) { infrared_mode_auto_ = v; }
	void set_visible_temp_correction(bool v) { visible_temp_correction_ = v; }
	void set_infrared_temp_correction(bool v) { infrared_temp_correction_ = v; }
	void set_visible_range(SI1145Range v) { visible_range_ = v; }
	void set_infrared_range(SI1145Range v) { infrared_range_ = v; }
	void set_visible_gain(uint8_t v) { visible_gain_ = v; }
	void set_infrared_gain(uint8_t v) { infrared_gain_ = v; }

	void setup() override;
	void dump_config() override;
	float get_setup_priority() const override;
	void update() override;

 protected:
	uint16_t read_visible_();
	uint16_t read_infrared_();
	float read_uvindex_();
	uint16_t read_temp_();

	bool begin_();
	void reset_();

	void write8_(uint8_t reg, uint8_t val);
	uint8_t read8_(uint8_t reg);
	uint16_t read16_(uint8_t reg);
	uint8_t write_param_(uint8_t p, uint8_t v);

	void auto_range_visible_(uint16_t read_value);
	void auto_range_infrared_(uint16_t read_value);
	void set_visible_gain_(uint8_t gain);
	void set_infrared_gain_(uint8_t gain);
	void set_visible_range_(uint8_t range);
	void set_infrared_range_(uint8_t range);

	sensor::Sensor *visible_sensor_{nullptr};
	sensor::Sensor *infrared_sensor_{nullptr};
	sensor::Sensor *uvindex_sensor_{nullptr};
	sensor::Sensor *illuminance_sensor_{nullptr};

	SI1145Range visible_range_ = SI1145Range::RANGE_LOW;
	SI1145Range infrared_range_ = SI1145Range::RANGE_LOW;
	uint8_t visible_gain_ = 0;
	uint8_t infrared_gain_ = 0;

	bool visible_mode_auto_ = true;
	bool infrared_mode_auto_ = true;
	bool visible_temp_correction_ = false;
	bool infrared_temp_correction_ = false;

	uint16_t temp_at_begin_ = 0;
};

}  // namespace si1145_sensor
}  // namespace esphome

