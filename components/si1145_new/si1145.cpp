#include "si1145.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace si1145_new {

static const char *const TAG = "si1145_new";

void SI1145NewComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SI1145 (new/raw)...");
  if (!this->begin_()) {
    this->mark_failed();
    ESP_LOGE(TAG, "SI1145 init failed");
  }
}

void SI1145NewComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SI1145 New (raw):");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with SI1145 failed");
  }
}

float SI1145NewComponent::get_setup_priority() const { return setup_priority::DATA; }

void SI1145NewComponent::update() {
  uint16_t vis = 0;
  uint16_t ir = 0;
  uint16_t uv_raw = 0;
  uint16_t vis_forced = 0;
  uint16_t ir_forced = 0;
  uint16_t uv_forced = 0;
  uint8_t response = 0;
  uint8_t chipstat = 0;
  uint8_t irqstat = 0;

  this->read8_(SI1145_REG_RESPONSE, response);
  this->read8_(SI1145_REG_CHIPSTAT, chipstat);
  this->read8_(SI1145_REG_IRQSTAT, irqstat);
  this->read16_(SI1145_REG_ALSVISDATA0, vis);
  this->read16_(SI1145_REG_ALSIRDATA0, ir);
  this->read16_(SI1145_REG_UVINDEX0, uv_raw);

  // If frame looks dead, force one fresh ALS measurement and read again.
  if (vis == 0 && ir == 0 && uv_raw == 0) {
    if (!this->write8_(SI1145_REG_COMMAND, SI1145_ALS_FORCE)) {
      ESP_LOGW(TAG, "ALS_FORCE command failed");
    }
    delay(25);
    this->read16_(SI1145_REG_ALSVISDATA0, vis_forced);
    this->read16_(SI1145_REG_ALSIRDATA0, ir_forced);
    this->read16_(SI1145_REG_UVINDEX0, uv_forced);
  }

  // Clear IRQ bits by writing back what was read.
  this->write8_(SI1145_REG_IRQSTAT, irqstat);

  ESP_LOGD(TAG,
           "raw resp=0x%02X chip=0x%02X irq=0x%02X vis=%u ir=%u uv=%u | forced vis=%u ir=%u uv=%u",
           response, chipstat, irqstat, vis, ir, uv_raw, vis_forced, ir_forced, uv_forced);

  if (vis == 0 && ir == 0 && uv_raw == 0 && (vis_forced || ir_forced || uv_forced)) {
    vis = vis_forced;
    ir = ir_forced;
    uv_raw = uv_forced;
  }

  if (this->visible_sensor_ != nullptr) {
    this->visible_sensor_->publish_state(static_cast<float>(vis));
  }
  if (this->infrared_sensor_ != nullptr) {
    this->infrared_sensor_->publish_state(static_cast<float>(ir));
  }
  if (this->uvindex_sensor_ != nullptr) {
    this->uvindex_sensor_->publish_state(uv_raw / 100.0f);
  }

  this->write8_(SI1145_REG_COMMAND, SI1145_NOP);
}

bool SI1145NewComponent::begin_() {
  uint8_t id = 0;
  if (!this->read8_(SI1145_REG_PARTID, id) || id != 0x45) {
    ESP_LOGE(TAG, "Unexpected PARTID: 0x%02X", id);
    return false;
  }

  this->reset_();

  // Adafruit reference initialization sequence.
  this->write8_(SI1145_REG_UCOEFF0, 0x29);
  this->write8_(SI1145_REG_UCOEFF1, 0x89);
  this->write8_(SI1145_REG_UCOEFF2, 0x02);
  this->write8_(SI1145_REG_UCOEFF3, 0x00);

  // ALS-only channels for UV/IR/VIS. Proximity is intentionally disabled.
  this->write_param_(SI1145_PARAM_CHLIST,
                     SI1145_PARAM_CHLIST_ENUV | SI1145_PARAM_CHLIST_ENALSIR | SI1145_PARAM_CHLIST_ENALSVIS);
  this->write8_(SI1145_REG_INTCFG, SI1145_REG_INTCFG_INTOE);
  this->write8_(SI1145_REG_IRQEN, SI1145_REG_IRQEN_ALSEVERYSAMPLE);

  this->write_param_(SI1145_PARAM_ALSIRADCMUX, SI1145_PARAM_ADCMUX_SMALLIR);
  this->write_param_(SI1145_PARAM_ALSIRADCGAIN, 0x00);
  this->write_param_(SI1145_PARAM_ALSIRADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
  this->write_param_(SI1145_PARAM_ALSIRADCMISC, 0x20);

  this->write_param_(SI1145_PARAM_ALSVISADCGAIN, 0x00);
  this->write_param_(SI1145_PARAM_ALSVISADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
  this->write_param_(SI1145_PARAM_ALSVISADCMISC, 0x20);

  this->write8_(SI1145_REG_MEASRATE0, 0xFF);
  this->write8_(SI1145_REG_COMMAND, SI1145_ALS_AUTO);

  uint8_t chlist = 0;
  uint8_t vis_gain = 0;
  uint8_t vis_misc = 0;
  uint8_t ir_gain = 0;
  uint8_t ir_misc = 0;
  uint8_t chipstat = 0;
  uint8_t response = 0;
  this->read_param_(SI1145_PARAM_CHLIST, chlist);
  this->read_param_(SI1145_PARAM_ALSVISADCGAIN, vis_gain);
  this->read_param_(SI1145_PARAM_ALSVISADCMISC, vis_misc);
  this->read_param_(SI1145_PARAM_ALSIRADCGAIN, ir_gain);
  this->read_param_(SI1145_PARAM_ALSIRADCMISC, ir_misc);
  this->read8_(SI1145_REG_CHIPSTAT, chipstat);
  this->read8_(SI1145_REG_RESPONSE, response);
  ESP_LOGI(TAG,
           "init rb chlist=0x%02X vis_gain=0x%02X vis_misc=0x%02X ir_gain=0x%02X ir_misc=0x%02X chip=0x%02X resp=0x%02X",
           chlist, vis_gain, vis_misc, ir_gain, ir_misc, chipstat, response);

  ESP_LOGI(TAG, "SI1145 init done");
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

bool SI1145NewComponent::write8_(uint8_t reg, uint8_t val) {
  const bool ok = this->write_byte(reg, val);
  ESP_LOGD(TAG, "i2c write reg=0x%02X val=0x%02X ok=%d", reg, val, ok ? 1 : 0);
  return ok;
}

bool SI1145NewComponent::read8_(uint8_t reg, uint8_t &val) {
  const bool ok = this->read_byte(reg, &val);
  ESP_LOGD(TAG, "i2c read8 reg=0x%02X val=0x%02X ok=%d", reg, val, ok ? 1 : 0);
  return ok;
}

bool SI1145NewComponent::read16_(uint8_t reg, uint16_t &val) {
  uint8_t write_reg = reg;
  uint8_t data[2] = {0, 0};
  const i2c::ErrorCode err = this->write_read(&write_reg, 1, data, 2);
  const bool ok = err == i2c::ERROR_OK;
  val = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  ESP_LOGD(TAG, "i2c read16 reg=0x%02X val=0x%04X ok=%d err=%d", reg, val, ok ? 1 : 0, static_cast<int>(err));
  return ok;
}

bool SI1145NewComponent::write_param_(uint8_t param, uint8_t value) {
  bool ok = true;
  ok &= this->write8_(SI1145_REG_PARAMWR, value);
  ok &= this->write8_(SI1145_REG_COMMAND, param | SI1145_PARAM_SET);

  uint8_t readback = 0;
  const bool rb_ok = this->read8_(SI1145_REG_PARAMRD, readback);
  ESP_LOGD(TAG, "param write p=0x%02X v=0x%02X readback=0x%02X ok=%d", param, value, readback, (ok && rb_ok) ? 1 : 0);
  return ok && rb_ok;
}

bool SI1145NewComponent::read_param_(uint8_t param, uint8_t &value) {
  bool ok = this->write8_(SI1145_REG_COMMAND, param | SI1145_PARAM_QUERY);
  bool rb_ok = this->read8_(SI1145_REG_PARAMRD, value);
  ESP_LOGD(TAG, "param read p=0x%02X v=0x%02X ok=%d", param, value, (ok && rb_ok) ? 1 : 0);
  return ok && rb_ok;
}

}  // namespace si1145_new
}  // namespace esphome
