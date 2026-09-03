#include "m5pm_imu.hpp"

#include "bmi2.h"
#include "bmi270.h"
#include "esp_rom_sys.h"
#include "m5pm_board.hpp"

namespace m5pm::imu {

std::int8_t Sampler::read_bus(std::uint8_t address, std::uint8_t* data, std::uint32_t length, void* context)
{
    if (context == nullptr || data == nullptr || length == 0) return BMI2_E_NULL_PTR;
    return i2c_bus_read_bytes(static_cast<i2c_bus_device_handle_t>(context), address, length, data) == ESP_OK
               ? BMI2_OK
               : BMI2_E_COM_FAIL;
}

std::int8_t Sampler::write_bus(std::uint8_t address, const std::uint8_t* data, std::uint32_t length, void* context)
{
    if (context == nullptr || data == nullptr || length == 0) return BMI2_E_NULL_PTR;
    return i2c_bus_write_bytes(static_cast<i2c_bus_device_handle_t>(context), address, length, data) == ESP_OK
               ? BMI2_OK
               : BMI2_E_COM_FAIL;
}

void Sampler::delay_us(std::uint32_t period, void*)
{
    esp_rom_delay_us(period);
}

esp_err_t Sampler::begin(i2c_bus_handle_t bus)
{
    if (bus == nullptr || ready_ || device_ != nullptr) return ESP_ERR_INVALID_STATE;
    device_ = i2c_bus_device_create(bus, m5pm::board::kImuAddress,
                                    static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard));
    if (device_ == nullptr) return ESP_ERR_NO_MEM;

    bmi_.intf            = BMI2_I2C_INTF;
    bmi_.read            = read_bus;
    bmi_.write           = write_bus;
    bmi_.delay_us        = delay_us;
    bmi_.read_write_len  = 30;
    bmi_.config_file_ptr = nullptr;
    bmi_.intf_ptr        = device_;
    if (bmi270_init(&bmi_) != BMI2_OK) return ESP_FAIL;

    bmi2_sens_config sensors[2] = {};
    sensors[0].type             = BMI2_ACCEL;
    sensors[1].type             = BMI2_GYRO;
    if (bmi270_get_sensor_config(sensors, 2, &bmi_) != BMI2_OK) return ESP_FAIL;
    sensors[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
    sensors[0].cfg.acc.bwp         = BMI2_ACC_OSR2_AVG2;
    sensors[0].cfg.acc.odr         = BMI2_ACC_ODR_100HZ;
    sensors[0].cfg.acc.range       = BMI2_ACC_RANGE_2G;
    sensors[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;
    sensors[1].cfg.gyr.noise_perf  = BMI2_POWER_OPT_MODE;
    sensors[1].cfg.gyr.bwp         = BMI2_GYR_OSR2_MODE;
    sensors[1].cfg.gyr.odr         = BMI2_GYR_ODR_100HZ;
    sensors[1].cfg.gyr.range       = BMI2_GYR_RANGE_2000;
    sensors[1].cfg.gyr.ois_range   = BMI2_GYR_OIS_2000;
    if (bmi270_set_sensor_config(sensors, 2, &bmi_) != BMI2_OK) return ESP_FAIL;
    const std::uint8_t enabled[] = {BMI2_ACCEL, BMI2_GYRO};
    if (bmi270_sensor_enable(enabled, sizeof(enabled), &bmi_) != BMI2_OK) return ESP_FAIL;
    ready_ = true;
    return ESP_OK;
}

esp_err_t Sampler::read(Sample* sample)
{
    if (!ready_ || sample == nullptr) return ESP_ERR_INVALID_STATE;
    bmi2_sens_data data = {};
    if (bmi2_get_sensor_data(&data, &bmi_) != BMI2_OK) return ESP_FAIL;
    sample->acceleration = data.acc;
    sample->gyroscope    = data.gyr;
    return ESP_OK;
}

}  // namespace m5pm::imu
