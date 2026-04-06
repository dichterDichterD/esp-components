#include "si1145.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace si1145_new {

static const char *const TAG = "si1145_new";
static const uint16_t SI1145_DARK_OFFSET = 270;

void SI1145NewComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SI1145...");
  if (!this->begin_()) {
    this->mark_failed();
  }
}

void SI1145NewComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SI1145:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with SI1145 failed");
  }
}

float SI1145NewComponent::get_setup_priority() const { return setup_priority::DATA; }

void SI1145NewComponent::update() {
  this->write8_(SI1145_REG_COMMAND, SI1145_PSALS_FORCE);
  delay(10);

  const uint8_t irq_status = this->read8_(SI1145_REG_IRQSTAT);
  this->write8_(SI1145_REG_IRQSTAT, irq_status);

  const uint16_t visible_raw = this->read_visible_raw_();
  const uint16_t infrared_raw = this->read_infrared_raw_();
  const uint16_t visible = this->read_visible_();
  const uint16_t infrared = this->read_infrared_();
  const float uv_index = this->read_uvindex_();
  const float lux = this->read_lux_(visible, infrared);

  ESP_LOGD(TAG, "irq=0x%02X vis_raw=%u ir_raw=%u vis=%u ir=%u uv=%.2f lux=%.1f", irq_status,
           visible_raw, infrared_raw, visible, infrared, uv_index, lux);

  if (this->visible_sensor_ != nullptr) {
    this->visible_sensor_->publish_state(visible);
  }
  if (this->infrared_sensor_ != nullptr) {
    this->infrared_sensor_->publish_state(infrared);
  }
  if (this->uvindex_sensor_ != nullptr) {
    this->uvindex_sensor_->publish_state(uv_index);
  }
  if (this->illuminance_sensor_ != nullptr) {
    this->illuminance_sensor_->publish_state(lux);
  }
  if (this->visible_raw_sensor_ != nullptr) {
    this->visible_raw_sensor_->publish_state(visible_raw);
  }
  if (this->infrared_raw_sensor_ != nullptr) {
    this->infrared_raw_sensor_->publish_state(infrared_raw);
  }

  this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
}

uint16_t SI1145NewComponent::read_visible_raw_() { return this->read16_(SI1145_REG_ALSVISDATA0); }

uint16_t SI1145NewComponent::read_infrared_raw_() { return this->read16_(SI1145_REG_ALSIRDATA0); }

uint16_t SI1145NewComponent::read_visible_() {
  const uint16_t raw = this->read_visible_raw_();
  return raw > SI1145_DARK_OFFSET ? (raw - SI1145_DARK_OFFSET) : 0;
}

uint16_t SI1145NewComponent::read_infrared_() {
  const uint16_t raw = this->read_infrared_raw_();
  return raw > SI1145_DARK_OFFSET ? (raw - SI1145_DARK_OFFSET) : 0;
}

float SI1145NewComponent::read_uvindex_() {
  const uint16_t uv_raw = this->read16_(SI1145_REG_UVINDEX0);
  return uv_raw / 100.0f;
}

float SI1145NewComponent::read_lux_(uint16_t visible, uint16_t infrared) {
  const float lux = 5.41f * visible - 0.08f * infrared;
  return lux < 0.0f ? 0.0f : lux;
}

bool SI1145NewComponent::begin_() {
  if (this->read8_(SI1145_REG_PARTID) != 0x45) {
    return false;
  }

  this->reset_();

  this->write8_(SI1145_REG_UCOEFF0, 0x29);
  this->write8_(SI1145_REG_UCOEFF1, 0x89);
  this->write8_(SI1145_REG_UCOEFF2, 0x02);
  this->write8_(SI1145_REG_UCOEFF3, 0x00);

  this->write_param_(SI1145_PARAM_CHLIST,
                     SI1145_PARAM_CHLIST_ENUV | SI1145_PARAM_CHLIST_ENALSIR |
                         SI1145_PARAM_CHLIST_ENALSVIS | SI1145_PARAM_CHLIST_ENPS1);
  this->write8_(SI1145_REG_INTCFG, SI1145_REG_INTCFG_INTOE);
  this->write8_(SI1145_REG_IRQEN, SI1145_REG_IRQEN_ALSEVERYSAMPLE);

  this->write8_(SI1145_REG_PSLED21, 0x03);
  this->write_param_(SI1145_PARAM_PS1ADCMUX, SI1145_PARAM_ADCMUX_LARGEIR);
  this->write_param_(SI1145_PARAM_PSLED12SEL, SI1145_PARAM_PSLED12SEL_PS1LED1);
  this->write_param_(SI1145_PARAM_PSADCGAIN, 0);
  this->write_param_(SI1145_PARAM_PSADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
  this->write_param_(SI1145_PARAM_PSADCMISC,
                     SI1145_PARAM_PSADCMISC_RANGE | SI1145_PARAM_PSADCMISC_PSMODE);

  this->write_param_(SI1145_PARAM_ALSIRADCMUX, SI1145_PARAM_ADCMUX_SMALLIR);
  this->write_param_(SI1145_PARAM_ALSIRADCGAIN, 0);
  this->write_param_(SI1145_PARAM_ALSIRADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
  this->write_param_(SI1145_PARAM_ALSIRADCMISC, 0);

  this->write_param_(SI1145_PARAM_ALSVISADCGAIN, 0);
  this->write_param_(SI1145_PARAM_ALSVISADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
  this->write_param_(SI1145_PARAM_ALSVISADCMISC, 0);

  this->write8_(SI1145_REG_MEASRATE0, 0xFF);
  this->write8_(SI1145_REG_COMMAND, SI1145_PSALS_AUTO);
  return true;
}

void SI1145NewComponent::reset_() {
  this->write8_(SI1145_REG_MEASRATE0, 0x00);
  this->write8_(SI1145_REG_MEASRATE1, 0x00);
  this->write8_(SI1145_REG_IRQEN, 0x00);
  this->write8_(SI1145_REG_IRQMODE1, 0x00);
  this->write8_(SI1145_REG_IRQMODE2, 0x00);
  this->write8_(SI1145_REG_INTCFG, 0x00);
  this->write8_(SI1145_REG_IRQSTAT, 0xFF);

  this->write8_(SI1145_REG_COMMAND, SI1145_RESET);
  delay(10);
  this->write8_(SI1145_REG_HWKEY, 0x17);
  delay(10);
}

void SI1145NewComponent::write8_(uint8_t reg, uint8_t val) { this->write_byte(reg, val); }

uint8_t SI1145NewComponent::read8_(uint8_t reg) {
  uint8_t d8 = 0;
  this->read_byte(reg, &d8);
  return d8;
}

uint16_t SI1145NewComponent::read16_(uint8_t reg) {
  const uint8_t low = this->read8_(reg);
  const uint8_t high = this->read8_(reg + 1);
  return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

uint8_t SI1145NewComponent::write_param_(uint8_t p, uint8_t v) {
  this->write8_(SI1145_REG_PARAMWR, v);
  this->write8_(SI1145_REG_COMMAND, p | SI1145_PARAM_SET);
  return this->read8_(SI1145_REG_PARAMRD);
}

}  // namespace si1145_new
}  // namespace esphome
