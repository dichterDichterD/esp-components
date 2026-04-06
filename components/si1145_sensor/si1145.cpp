#include "si1145.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace si1145_sensor {

static const char *const TAG = "si1145_sensor";

inline float visible_temp_correction(uint16_t value, uint8_t range, uint8_t gain, uint16_t temp,
																		 uint16_t temp_at_begin) {
	float vis = value;
	if (range == SI1145Range::RANGE_LOW) {
		switch (gain) {
			case 0:
				vis = vis - 0.3f * (temp - temp_at_begin) / 35.0f;
				break;
			case 1:
				vis = vis - 0.11f * (temp - temp_at_begin) / 35.0f;
				break;
			case 2:
				vis = vis - 0.06f * (temp - temp_at_begin) / 35.0f;
				break;
			case 3:
				vis = vis - 0.03f * (temp - temp_at_begin) / 35.0f;
				break;
			case 4:
				vis = vis - 0.01f * (temp - temp_at_begin) / 35.0f;
				break;
			case 5:
				vis = vis - 0.008f * (temp - temp_at_begin) / 35.0f;
				break;
			case 6:
				vis = vis - 0.007f * (temp - temp_at_begin) / 35.0f;
				break;
			case 7:
				vis = vis - 0.008f * (temp - temp_at_begin) / 35.0f;
				break;
			default:
				break;
		}
	}
	return vis;
}

inline float infrared_temp_correction(uint16_t value, uint8_t range, uint8_t gain, uint16_t temp,
																			uint16_t temp_at_begin) {
	float ir = value;
	if (range == SI1145Range::RANGE_LOW) {
		switch (gain) {
			case 0:
				ir = ir - 0.3f * (temp - temp_at_begin) / 35.0f;
				break;
			case 1:
				ir = ir - 0.06f * (temp - temp_at_begin) / 35.0f;
				break;
			case 2:
				ir = ir - 0.03f * (temp - temp_at_begin) / 35.0f;
				break;
			case 3:
				ir = ir - 0.01f * (temp - temp_at_begin) / 35.0f;
				break;
			default:
				break;
		}
	}
	return ir;
}

inline float illumination_combine_sensors(float vis_value, uint8_t vis_range, uint8_t vis_gain, float ir_value,
																					uint8_t ir_range, uint8_t ir_gain) {
	float range_vis = (vis_range == SI1145Range::RANGE_LOW) ? 1.0f : 14.5f;
	float range_ir = (ir_range == SI1145Range::RANGE_LOW) ? 1.0f : 14.5f;
	float lux = (5.41f * vis_value * range_vis) / (1 << vis_gain) + (-0.08f * ir_value * range_ir) / (1 << ir_gain);
	return lux < 0.0f ? 0.0f : lux;
}

inline float apply_range_and_gain(float value, uint8_t range, uint8_t gain) {
	float range_factor = (range == SI1145Range::RANGE_LOW) ? 1.0f : 14.5f;
	return (value * range_factor) / (1 << gain);
}

void SI1145SensorComponent::setup() {
	ESP_LOGCONFIG(TAG, "Setting up SI1145 sensor...");
	if (!this->begin_()) {
		this->mark_failed();
		return;
	}

	this->set_visible_gain_(this->visible_gain_);
	this->set_visible_range_(this->visible_range_);
	this->set_infrared_gain_(this->infrared_gain_);
	this->set_infrared_range_(this->infrared_range_);
}

void SI1145SensorComponent::dump_config() {
	ESP_LOGCONFIG(TAG, "SI1145 Sensor:");
	LOG_I2C_DEVICE(this);
	if (this->is_failed()) {
		ESP_LOGE(TAG, "Communication with SI1145 failed");
	}
}

float SI1145SensorComponent::get_setup_priority() const { return setup_priority::DATA; }

void SI1145SensorComponent::update() {
	// Device runs in PSALS_AUTO mode after setup, so we can read the latest sample
	// directly without blocking the main loop with an extra conversion delay.

	float vis;
	float ir;
	float tp;
	uint8_t resp = this->read8_(SI1145_REG_RESPONSE);

	switch (resp) {
		case 0x8C:
			vis = OVERFLOW_VALUE;
			ir = this->read_infrared_();
			tp = this->read_temp_();
			this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
			break;
		case 0x8D:
			ir = OVERFLOW_VALUE;
			vis = this->read_visible_();
			tp = this->read_temp_();
			this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
			break;
		case 0x8E:
			vis = this->read_visible_();
			ir = this->read_infrared_();
			tp = this->temp_at_begin_;
			this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
			break;
		default:
			vis = this->read_visible_();
			ir = this->read_infrared_();
			tp = this->read_temp_();
			break;
	}

	const uint16_t visible_ar = vis;
	const uint16_t infrared_ar = ir;

	uint8_t irq_status = this->read8_(SI1145_REG_IRQSTAT);
	this->write8_(SI1145_REG_IRQSTAT, irq_status);

	if (this->visible_temp_correction_ && vis != OVERFLOW_VALUE) {
		vis = visible_temp_correction(vis, this->visible_range_, this->visible_gain_, tp, this->temp_at_begin_);
	}
	if (this->infrared_temp_correction_ && ir != OVERFLOW_VALUE) {
		ir = infrared_temp_correction(ir, this->infrared_range_, this->infrared_gain_, tp, this->temp_at_begin_);
	}

	if (this->visible_sensor_ != nullptr && vis != OVERFLOW_VALUE) {
		this->visible_sensor_->publish_state(apply_range_and_gain(vis, this->visible_range_, this->visible_gain_));
	}
	if (this->infrared_sensor_ != nullptr && ir != OVERFLOW_VALUE) {
		this->infrared_sensor_->publish_state(apply_range_and_gain(ir, this->infrared_range_, this->infrared_gain_));
	}
	if (this->uvindex_sensor_ != nullptr && vis != OVERFLOW_VALUE && ir != OVERFLOW_VALUE) {
		this->uvindex_sensor_->publish_state(this->read_uvindex_());
	}
	if (this->illuminance_sensor_ != nullptr && vis != OVERFLOW_VALUE && ir != OVERFLOW_VALUE) {
		float lux = illumination_combine_sensors(vis, this->visible_range_, this->visible_gain_, ir, this->infrared_range_,
																						 this->infrared_gain_);
		this->illuminance_sensor_->publish_state(lux);
	}

	if (this->visible_mode_auto_) {
		this->auto_range_visible_(visible_ar);
	}
	if (this->infrared_mode_auto_) {
		this->auto_range_infrared_(infrared_ar);
	}
	this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
}

uint16_t SI1145SensorComponent::read_visible_() {
	uint16_t r = this->read16_(SI1145_REG_ALSVISDATA0);
	uint16_t vatzero = VALUE_AT_ZERO_HIGH;
	if (this->visible_range_ == SI1145Range::RANGE_LOW) {
		vatzero = VALUE_AT_ZERO_LOW;
	}
	if (r <= vatzero) {
		return 0;
	}
	return r - vatzero;
}

uint16_t SI1145SensorComponent::read_infrared_() {
	uint16_t r = this->read16_(SI1145_REG_ALSIRDATA0);
	uint16_t vatzero = VALUE_AT_ZERO_HIGH;
	if (this->infrared_range_ == SI1145Range::RANGE_LOW) {
		vatzero = VALUE_AT_ZERO_LOW;
	}
	if (r <= vatzero) {
		return 0;
	}
	return r - vatzero;
}

uint16_t SI1145SensorComponent::read_temp_() {
	uint16_t temp = this->read8_(SI1145_REG_UVINDEX0);
	temp |= (this->read8_(SI1145_REG_UVINDEX1) << 8);
	return temp;
}

uint8_t SI1145SensorComponent::read_uvindex_() {
	int uv = this->read16_(SI1145_REG_UVINDEX0);
	return static_cast<uint8_t>(std::floor(uv / 100.0));
}

bool SI1145SensorComponent::begin_() {
	if (this->read8_(SI1145_REG_PARTID) != 0x45) {
		return false;
	}

	this->reset_();

	this->write8_(SI1145_REG_UCOEFF0, 0x29);
	this->write8_(SI1145_REG_UCOEFF1, 0x89);
	this->write8_(SI1145_REG_UCOEFF2, 0x02);
	this->write8_(SI1145_REG_UCOEFF3, 0x00);

	this->write_param_(SI1145_PARAM_CHLIST, SI1145_PARAM_CHLIST_ENUV | SI1145_PARAM_CHLIST_ENALSIR |
																							SI1145_PARAM_CHLIST_ENALSVIS | SI1145_PARAM_CHLIST_ENPS1);
	this->write8_(SI1145_REG_INTCFG, SI1145_REG_INTCFG_INTOE);
	this->write8_(SI1145_REG_IRQEN, SI1145_REG_IRQEN_ALSEVERYSAMPLE);

	this->write8_(SI1145_REG_PSLED21, 0x03);
	this->write_param_(SI1145_PARAM_PS1ADCMUX, SI1145_PARAM_ADCMUX_LARGEIR);
	this->write_param_(SI1145_PARAM_PSLED12SEL, SI1145_PARAM_PSLED12SEL_PS1LED1);
	this->write_param_(SI1145_PARAM_PSADCGAIN, 0);
	this->write_param_(SI1145_PARAM_PSADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
	this->write_param_(SI1145_PARAM_PSADCMISC, SI1145_PARAM_PSADCMISC_RANGE | SI1145_PARAM_PSADCMISC_PSMODE);

	this->write_param_(SI1145_PARAM_ALSIRADCMUX, SI1145_PARAM_ADCMUX_SMALLIR);
	this->write_param_(SI1145_PARAM_ALSIRADCGAIN, this->infrared_gain_);
	this->write_param_(SI1145_PARAM_ALSIRADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
	this->write_param_(SI1145_PARAM_ALSIRADCMISC, this->infrared_range_);

	this->write_param_(SI1145_PARAM_ALSVISADCGAIN, this->visible_gain_);
	this->write_param_(SI1145_PARAM_ALSVISADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
	this->write_param_(SI1145_PARAM_ALSVISADCMISC, this->visible_range_);

	this->write8_(SI1145_REG_MEASRATE0, 0xFF);
	this->write8_(SI1145_REG_COMMAND, SI1145_PSALS_AUTO);

	this->temp_at_begin_ = this->read_temp_();
	return true;
}

void SI1145SensorComponent::reset_() {
	this->write8_(SI1145_REG_MEASRATE0, 0x00);
	this->write8_(SI1145_REG_MEASRATE1, 0x00);
	this->write8_(SI1145_REG_IRQEN, 0x00);
	this->write8_(SI1145_REG_IRQMODE1, 0x00);
	this->write8_(SI1145_REG_IRQMODE2, 0x00);
	this->write8_(SI1145_REG_INTCFG, 0x00);
	this->write8_(SI1145_REG_IRQSTAT, 0xFF);

	this->write8_(SI1145_REG_COMMAND, SI1145_RESET);
	if (!this->wait_for_part_id_(0x45, 500)) {
		ESP_LOGW(TAG, "Timeout waiting for SI1145 after reset command");
	}
	this->write8_(SI1145_REG_HWKEY, 0x17);
	if (!this->wait_for_part_id_(0x45, 500)) {
		ESP_LOGW(TAG, "Timeout waiting for SI1145 after HWKEY");
	}
}

void SI1145SensorComponent::set_visible_gain_(uint8_t gain) { this->write_param_(SI1145_PARAM_ALSVISADCGAIN, gain); }

void SI1145SensorComponent::set_infrared_gain_(uint8_t gain) { this->write_param_(SI1145_PARAM_ALSIRADCGAIN, gain); }

void SI1145SensorComponent::set_visible_range_(uint8_t range) {
	this->write_param_(SI1145_PARAM_ALSVISADCMISC, range);
}

void SI1145SensorComponent::set_infrared_range_(uint8_t range) {
	this->write_param_(SI1145_PARAM_ALSIRADCMISC, range);
}

void SI1145SensorComponent::auto_range_visible_(uint16_t read_value) {
	if (read_value > 25000) {
		if (this->visible_gain_ == 0) {
			if (this->visible_range_ == SI1145Range::RANGE_LOW) {
				this->visible_range_ = SI1145Range::RANGE_HIGH;
				this->set_visible_range_(this->visible_range_);
			}
		} else {
			this->visible_gain_ -= 1;
			this->set_visible_gain_(this->visible_gain_);
		}
	} else if (read_value < 1500) {
		if (this->visible_gain_ < 7) {
			this->visible_gain_ += 1;
			this->set_visible_gain_(this->visible_gain_);
		} else if (this->visible_range_ == SI1145Range::RANGE_HIGH) {
			this->visible_range_ = SI1145Range::RANGE_LOW;
			this->visible_gain_ = 0;
			this->set_visible_range_(this->visible_range_);
			this->set_visible_gain_(this->visible_gain_);
		}
	}
}

void SI1145SensorComponent::auto_range_infrared_(uint16_t read_value) {
	if (read_value > 65000 && this->infrared_range_ == SI1145Range::RANGE_HIGH) {
		this->infrared_range_ = SI1145Range::RANGE_LOW;
		this->infrared_gain_ = 0;
		this->set_infrared_range_(this->infrared_range_);
		this->set_infrared_gain_(this->infrared_gain_);
	} else if (read_value > 25000) {
		if (this->infrared_gain_ == 0) {
			if (this->infrared_range_ == SI1145Range::RANGE_LOW) {
				this->infrared_range_ = SI1145Range::RANGE_HIGH;
				this->set_infrared_range_(this->infrared_range_);
			}
		} else {
			this->infrared_gain_ -= 1;
			this->set_infrared_gain_(this->infrared_gain_);
		}
	} else if (read_value < 1500) {
		if (this->infrared_gain_ < 7) {
			this->infrared_gain_ += 1;
			this->set_infrared_gain_(this->infrared_gain_);
		} else if (this->infrared_range_ == SI1145Range::RANGE_HIGH) {
			this->infrared_range_ = SI1145Range::RANGE_LOW;
			this->infrared_gain_ = 0;
			this->set_infrared_range_(this->infrared_range_);
			this->set_infrared_gain_(this->infrared_gain_);
		}
	}
}

void SI1145SensorComponent::write8_(uint8_t reg, uint8_t val) { this->write_byte(reg, val); }

uint8_t SI1145SensorComponent::read8_(uint8_t reg) {
	uint8_t d8 = 0;
	this->read_byte(reg, &d8);
	return d8;
}

uint16_t SI1145SensorComponent::read16_(uint8_t reg) {
	uint16_t d16 = 0;
	this->read_byte_16(reg, &d16);
	return (d16 >> 8) | ((d16 & 0xFF) << 8);
}

uint8_t SI1145SensorComponent::write_param_(uint8_t p, uint8_t v) {
	this->write8_(SI1145_REG_PARAMWR, v);
	this->write8_(SI1145_REG_COMMAND, p | SI1145_PARAM_SET);
	return this->read8_(SI1145_REG_PARAMRD);
}

bool SI1145SensorComponent::wait_for_part_id_(uint8_t expected, uint16_t max_attempts) {
	for (uint16_t attempt = 0; attempt < max_attempts; attempt++) {
		if (this->read8_(SI1145_REG_PARTID) == expected) {
			return true;
		}
	}
	return false;
}

}  // namespace si1145_sensor
}  // namespace esphome

