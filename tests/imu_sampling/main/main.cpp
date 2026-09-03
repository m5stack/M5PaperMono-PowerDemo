#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "m5pm_imu.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr char kTag[]                = "imu_sampling";
constexpr std::uint32_t kSampleCount = 9000;
}  // namespace

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) m5pm::phase_test::fail("test_support", ESP_FAIL);
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok())
        m5pm::phase_test::fail(baseline_result.step, baseline_result.error, baseline_result.raw_error);
    const auto power_result = baseline.power().restore_imu();
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    static m5pm::imu::Sampler imu;
    if (imu.begin(baseline.bus()) != ESP_OK) m5pm::phase_test::fail("imu_begin", ESP_FAIL);
    m5pm::imu::Sample sample = {};
    if (imu.read(&sample) != ESP_OK) m5pm::phase_test::fail("imu_first_sample", ESP_FAIL);
    ESP_LOGI(kTag,
             "BOARD_OK BASELINE=l2-awake IMU=bmi270 ACCEL=on GYRO=on ODR_HZ=100 READ_PERIOD_MS=10 WAKE_INTERRUPT=0 "
             "DISPLAY_REFRESH=0");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    TickType_t next_sample = xTaskGetTickCount();
    for (std::uint32_t index = 0; index < kSampleCount; ++index) {
        vTaskDelayUntil(&next_sample, pdMS_TO_TICKS(10));
        if (imu.read(&sample) != ESP_OK) m5pm::phase_test::fail("imu_sample", ESP_FAIL);
    }
    m5pm::phase_test::pass_and_hold();
}
