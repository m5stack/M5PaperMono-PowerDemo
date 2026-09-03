#include "m5pm_imu_wakeup.hpp"

#include "bmi2.h"
#include "bmi270.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "m5pm_board.hpp"

namespace m5pm::wakeup {
namespace {

constexpr std::uint32_t kI2cSpeed = static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard);
constexpr char kTag[]             = "m5pm_imu_wakeup";

bool sensor_configuration_matches(const bmi2_sens_config* sensors, ImuWakeProfile profile)
{
    const bool low_power =
        sensors[0].cfg.acc.filter_perf == BMI2_POWER_OPT_MODE && sensors[0].cfg.acc.bwp == BMI2_ACC_OSR4_AVG1 &&
        sensors[0].cfg.acc.odr == BMI2_ACC_ODR_50HZ && sensors[0].cfg.acc.range == BMI2_ACC_RANGE_2G &&
        sensors[1].cfg.gyr.filter_perf == BMI2_POWER_OPT_MODE && sensors[1].cfg.gyr.noise_perf == BMI2_POWER_OPT_MODE &&
        sensors[1].cfg.gyr.bwp == BMI2_GYR_OSR4_MODE && sensors[1].cfg.gyr.odr == BMI2_GYR_ODR_25HZ &&
        sensors[1].cfg.gyr.range == BMI2_GYR_RANGE_2000 && sensors[1].cfg.gyr.ois_range == BMI2_GYR_OIS_2000;
    const bool reference =
        sensors[0].cfg.acc.filter_perf == BMI2_PERF_OPT_MODE && sensors[0].cfg.acc.bwp == BMI2_ACC_OSR2_AVG2 &&
        sensors[0].cfg.acc.odr == BMI2_ACC_ODR_100HZ && sensors[0].cfg.acc.range == BMI2_ACC_RANGE_2G &&
        sensors[1].cfg.gyr.filter_perf == BMI2_PERF_OPT_MODE && sensors[1].cfg.gyr.noise_perf == BMI2_POWER_OPT_MODE &&
        sensors[1].cfg.gyr.bwp == BMI2_GYR_OSR2_MODE && sensors[1].cfg.gyr.odr == BMI2_GYR_ODR_100HZ &&
        sensors[1].cfg.gyr.range == BMI2_GYR_RANGE_2000 && sensors[1].cfg.gyr.ois_range == BMI2_GYR_OIS_2000;
    const bool matches = profile == ImuWakeProfile::low_power ? low_power : reference;
    if (!matches) {
        ESP_LOGE(kTag,
                 "Sensor config mismatch profile=%s: acc expected=%u/%u/%u/%u actual=%u/%u/%u/%u; "
                 "gyr expected=%u/%u/%u/%u/%u/%u actual=%u/%u/%u/%u/%u/%u",
                 profile == ImuWakeProfile::low_power ? "low_power" : "reference",
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_ACC_ODR_50HZ : BMI2_ACC_ODR_100HZ),
                 static_cast<unsigned>(BMI2_ACC_RANGE_2G),
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_POWER_OPT_MODE : BMI2_PERF_OPT_MODE),
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_ACC_OSR4_AVG1 : BMI2_ACC_OSR2_AVG2),
                 static_cast<unsigned>(sensors[0].cfg.acc.odr), static_cast<unsigned>(sensors[0].cfg.acc.range),
                 static_cast<unsigned>(sensors[0].cfg.acc.filter_perf), static_cast<unsigned>(sensors[0].cfg.acc.bwp),
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_GYR_ODR_25HZ : BMI2_GYR_ODR_100HZ),
                 static_cast<unsigned>(BMI2_GYR_RANGE_2000),
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_POWER_OPT_MODE : BMI2_PERF_OPT_MODE),
                 static_cast<unsigned>(BMI2_POWER_OPT_MODE),
                 static_cast<unsigned>(profile == ImuWakeProfile::low_power ? BMI2_GYR_OSR4_MODE : BMI2_GYR_OSR2_MODE),
                 static_cast<unsigned>(BMI2_GYR_OIS_2000), static_cast<unsigned>(sensors[1].cfg.gyr.odr),
                 static_cast<unsigned>(sensors[1].cfg.gyr.range), static_cast<unsigned>(sensors[1].cfg.gyr.filter_perf),
                 static_cast<unsigned>(sensors[1].cfg.gyr.noise_perf), static_cast<unsigned>(sensors[1].cfg.gyr.bwp),
                 static_cast<unsigned>(sensors[1].cfg.gyr.ois_range));
    }
    return matches;
}

bool any_motion_configuration_matches(const bmi2_sens_config& config, std::uint8_t duration, std::uint8_t threshold)
{
    const auto& motion = config.cfg.any_motion;
    const bool matches = motion.threshold == threshold && motion.duration == duration &&
                         motion.select_x == BMI2_ENABLE && motion.select_y == BMI2_ENABLE &&
                         motion.select_z == BMI2_ENABLE;
    if (!matches) {
        ESP_LOGE(kTag, "Any-motion config mismatch: expected=%u/%u/%u/%u/%u actual=%u/%u/%u/%u/%u",
                 static_cast<unsigned>(duration), static_cast<unsigned>(threshold), static_cast<unsigned>(BMI2_ENABLE),
                 static_cast<unsigned>(BMI2_ENABLE), static_cast<unsigned>(BMI2_ENABLE),
                 static_cast<unsigned>(motion.duration), static_cast<unsigned>(motion.threshold),
                 static_cast<unsigned>(motion.select_x), static_cast<unsigned>(motion.select_y),
                 static_cast<unsigned>(motion.select_z));
    }
    return matches;
}

bool interrupt_configuration_matches(const bmi2_int_pin_config& interrupt)
{
    const auto& pin    = interrupt.pin_cfg[0];
    const bool matches = pin.lvl == BMI2_INT_ACTIVE_LOW && pin.od == BMI2_INT_PUSH_PULL &&
                         pin.output_en == BMI2_INT_OUTPUT_ENABLE && pin.input_en == BMI2_INT_INPUT_DISABLE &&
                         interrupt.int_latch == BMI2_INT_NON_LATCH;
    if (!matches) {
        ESP_LOGE(kTag, "INT1 config mismatch: expected=%u/%u/%u/%u/%u actual=%u/%u/%u/%u/%u",
                 static_cast<unsigned>(BMI2_INT_ACTIVE_LOW), static_cast<unsigned>(BMI2_INT_PUSH_PULL),
                 static_cast<unsigned>(BMI2_INT_OUTPUT_ENABLE), static_cast<unsigned>(BMI2_INT_INPUT_DISABLE),
                 static_cast<unsigned>(BMI2_INT_NON_LATCH), static_cast<unsigned>(pin.lvl),
                 static_cast<unsigned>(pin.od), static_cast<unsigned>(pin.output_en),
                 static_cast<unsigned>(pin.input_en), static_cast<unsigned>(interrupt.int_latch));
    }
    return matches;
}

}  // namespace

ImuResult ImuWakeup::api_result(std::int8_t raw, const char* step)
{
    return {raw == BMI2_OK ? ESP_OK : ESP_FAIL, static_cast<int>(raw), step};
}

std::int8_t ImuWakeup::read(std::uint8_t address, std::uint8_t* data, std::uint32_t length, void* context)
{
    if (context == nullptr || data == nullptr || length == 0U) {
        return BMI2_E_NULL_PTR;
    }
    const auto device = static_cast<i2c_bus_device_handle_t>(context);
    return i2c_bus_read_bytes(device, address, length, data) == ESP_OK ? BMI2_OK : BMI2_E_COM_FAIL;
}

std::int8_t ImuWakeup::write(std::uint8_t address, const std::uint8_t* data, std::uint32_t length, void* context)
{
    if (context == nullptr || data == nullptr || length == 0U) {
        return BMI2_E_NULL_PTR;
    }
    const auto device = static_cast<i2c_bus_device_handle_t>(context);
    return i2c_bus_write_bytes(device, address, length, data) == ESP_OK ? BMI2_OK : BMI2_E_COM_FAIL;
}

void ImuWakeup::delay_us(std::uint32_t period, void*)
{
    esp_rom_delay_us(period);
}

ImuResult ImuWakeup::read_retained_status(i2c_bus_handle_t bus, ImuStatus* status)
{
    if (bus == nullptr || status == nullptr) {
        return {ESP_ERR_INVALID_ARG, ESP_ERR_INVALID_ARG, "imu_retained_argument"};
    }

    i2c_bus_device_handle_t device = i2c_bus_device_create(bus, m5pm::board::kImuAddress, kI2cSpeed);
    if (device == nullptr) {
        return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM, "imu_retained_device_create"};
    }

    std::uint8_t data[2]          = {};
    const esp_err_t error         = i2c_bus_read_bytes(device, BMI2_INT_STATUS_0_ADDR, sizeof(data), data);
    const esp_err_t cleanup_error = i2c_bus_device_delete(&device);
    if (error != ESP_OK) {
        return {error, static_cast<int>(error), "imu_retained_status"};
    }
    if (cleanup_error != ESP_OK) {
        return {cleanup_error, static_cast<int>(cleanup_error), "imu_retained_device_delete"};
    }

    status->raw        = static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8U);
    status->any_motion = (status->raw & BMI270_ANY_MOT_STATUS_MASK) != 0U;
    return {};
}

ImuResult ImuWakeup::begin(i2c_bus_handle_t bus)
{
    if (bus == nullptr || initialized_ || device_ != nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "imu_begin_state"};
    }

    device_ = i2c_bus_device_create(bus, m5pm::board::kImuAddress, kI2cSpeed);
    if (device_ == nullptr) {
        return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM, "imu_device_create"};
    }

    bmi_.intf            = BMI2_I2C_INTF;
    bmi_.read            = read;
    bmi_.write           = write;
    bmi_.delay_us        = delay_us;
    bmi_.read_write_len  = 30;
    bmi_.config_file_ptr = nullptr;
    bmi_.intf_ptr        = device_;

    const std::int8_t raw = bmi270_init(&bmi_);
    if (raw != BMI2_OK) {
        i2c_bus_device_delete(&device_);
        return api_result(raw, "imu_initialize");
    }
    initialized_ = true;
    return {};
}

ImuResult ImuWakeup::configure_any_motion(ImuWakeProfile profile, std::uint8_t duration, std::uint8_t threshold)
{
    if (!initialized_ || device_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "imu_configure_state"};
    }
    if (duration != 10U || threshold != 30U) {
        return {ESP_ERR_INVALID_ARG, ESP_ERR_INVALID_ARG, "imu_configure_parameters"};
    }

    bmi2_sens_config sensors[2] = {};
    sensors[0].type             = BMI2_ACCEL;
    sensors[1].type             = BMI2_GYRO;
    std::int8_t raw             = bmi270_get_sensor_config(sensors, 2, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_sensor_config_get");

    sensors[0].cfg.acc.filter_perf = profile == ImuWakeProfile::low_power ? BMI2_POWER_OPT_MODE : BMI2_PERF_OPT_MODE;
    sensors[0].cfg.acc.bwp         = profile == ImuWakeProfile::low_power ? BMI2_ACC_OSR4_AVG1 : BMI2_ACC_OSR2_AVG2;
    sensors[0].cfg.acc.odr         = profile == ImuWakeProfile::low_power ? BMI2_ACC_ODR_50HZ : BMI2_ACC_ODR_100HZ;
    sensors[0].cfg.acc.range       = BMI2_ACC_RANGE_2G;
    sensors[1].cfg.gyr.filter_perf = profile == ImuWakeProfile::low_power ? BMI2_POWER_OPT_MODE : BMI2_PERF_OPT_MODE;
    sensors[1].cfg.gyr.noise_perf  = BMI2_POWER_OPT_MODE;
    sensors[1].cfg.gyr.bwp         = profile == ImuWakeProfile::low_power ? BMI2_GYR_OSR4_MODE : BMI2_GYR_OSR2_MODE;
    sensors[1].cfg.gyr.odr         = profile == ImuWakeProfile::low_power ? BMI2_GYR_ODR_25HZ : BMI2_GYR_ODR_100HZ;
    sensors[1].cfg.gyr.range       = BMI2_GYR_RANGE_2000;
    sensors[1].cfg.gyr.ois_range   = BMI2_GYR_OIS_2000;
    raw                            = bmi270_set_sensor_config(sensors, 2, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_sensor_config_set");

    bmi2_sens_config readback_sensors[2] = {};
    readback_sensors[0].type             = BMI2_ACCEL;
    readback_sensors[1].type             = BMI2_GYRO;
    raw                                  = bmi270_get_sensor_config(readback_sensors, 2, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_sensor_readback");
    if (!sensor_configuration_matches(readback_sensors, profile)) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "imu_sensor_configuration_verify"};
    }

    const std::uint8_t low_power_sensors[] = {BMI2_ACCEL, BMI2_ANY_MOTION};
    const std::uint8_t reference_sensors[] = {BMI2_ACCEL, BMI2_GYRO, BMI2_WRIST_WEAR_WAKE_UP, BMI2_ANY_MOTION};
    const std::uint8_t* enabled_sensors = profile == ImuWakeProfile::low_power ? low_power_sensors : reference_sensors;
    const std::uint8_t enabled_count =
        profile == ImuWakeProfile::low_power ? sizeof(low_power_sensors) : sizeof(reference_sensors);
    raw = bmi270_sensor_enable(enabled_sensors, enabled_count, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_sensor_enable");

    bmi2_sens_config any_motion = {};
    any_motion.type             = BMI2_ANY_MOTION;
    raw                         = bmi270_get_sensor_config(&any_motion, 1, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_any_motion_get");
    any_motion.cfg.any_motion.threshold = threshold;
    any_motion.cfg.any_motion.duration  = duration;
    any_motion.cfg.any_motion.select_x  = BMI2_ENABLE;
    any_motion.cfg.any_motion.select_y  = BMI2_ENABLE;
    any_motion.cfg.any_motion.select_z  = BMI2_ENABLE;
    raw                                 = bmi270_set_sensor_config(&any_motion, 1, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_any_motion_set");

    bmi2_sens_config readback_motion = {};
    readback_motion.type             = BMI2_ANY_MOTION;
    raw                              = bmi270_get_sensor_config(&readback_motion, 1, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_any_motion_readback");
    if (!any_motion_configuration_matches(readback_motion, duration, threshold)) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "imu_any_motion_configuration_verify"};
    }

    raw = bmi2_map_feat_int(BMI2_ANY_MOTION, BMI2_INT1, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_any_motion_map");

    bmi2_int_pin_config interrupt  = {};
    interrupt.pin_type             = BMI2_INT1;
    interrupt.pin_cfg[0].lvl       = BMI2_INT_ACTIVE_LOW;
    interrupt.pin_cfg[0].od        = BMI2_INT_PUSH_PULL;
    interrupt.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    interrupt.pin_cfg[0].input_en  = BMI2_INT_INPUT_DISABLE;
    interrupt.int_latch            = BMI2_INT_NON_LATCH;
    raw                            = bmi2_set_int_pin_config(&interrupt, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_int1_configure");

    bmi2_int_pin_config readback_interrupt = {};
    readback_interrupt.pin_type            = BMI2_INT1;
    raw                                    = bmi2_get_int_pin_config(&readback_interrupt, &bmi_);
    if (raw != BMI2_OK) return api_result(raw, "imu_int1_readback");
    if (!interrupt_configuration_matches(readback_interrupt)) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "imu_int1_configuration_verify"};
    }

    std::uint8_t power_control  = 0;
    const esp_err_t power_error = i2c_bus_read_byte(device_, BMI2_PWR_CTRL_ADDR, &power_control);
    if (power_error != ESP_OK) {
        return {power_error, static_cast<int>(power_error), "imu_power_readback"};
    }
    constexpr std::uint8_t kSensorPowerMask =
        BMI2_AUX_EN_MASK | BMI2_GYR_EN_MASK | BMI2_ACC_EN_MASK | BMI2_TEMP_EN_MASK;
    const std::uint8_t expected_power =
        profile == ImuWakeProfile::low_power ? BMI2_ACC_EN_MASK : BMI2_ACC_EN_MASK | BMI2_GYR_EN_MASK;
    if ((power_control & kSensorPowerMask) != expected_power) {
        return {ESP_ERR_INVALID_RESPONSE, power_control, "imu_power_verify"};
    }

    std::uint16_t discarded_status = 0;
    raw                            = bmi2_get_int_status(&discarded_status, &bmi_);
    return api_result(raw, "imu_status_clear");
}

ImuResult ImuWakeup::release_bus()
{
    if (!initialized_ || device_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "imu_release_state"};
    }
    const esp_err_t error = i2c_bus_device_delete(&device_);
    initialized_          = false;
    bmi_.intf_ptr         = nullptr;
    return {error, static_cast<int>(error), "imu_device_delete"};
}

}  // namespace m5pm::wakeup
