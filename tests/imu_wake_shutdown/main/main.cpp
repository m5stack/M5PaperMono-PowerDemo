#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"
#include "m5pm_imu_wakeup.hpp"
#include "m5pm_power.hpp"
#include "m5pm_test_support.hpp"

// Set to 1 for the full reference sensor load, or 0 for the low-power load.
#define M5PM_IMU_WAKE_REFERENCE_PROFILE 0

namespace {
constexpr char kTag[]              = "imu_wake_shutdown";
constexpr TickType_t kPrepareDelay = pdMS_TO_TICKS(3000);
#if M5PM_IMU_WAKE_REFERENCE_PROFILE
constexpr auto kImuWakeProfile       = m5pm::wakeup::ImuWakeProfile::reference;
constexpr char kImuWakeProfileName[] = "reference";
#else
constexpr auto kImuWakeProfile       = m5pm::wakeup::ImuWakeProfile::low_power;
constexpr char kImuWakeProfileName[] = "low_power";
#endif
constexpr std::uint8_t kAllPm1WakeSources = M5PM1_WAKE_SRC_TIM | M5PM1_WAKE_SRC_VIN | M5PM1_WAKE_SRC_PWRBTN |
                                            M5PM1_WAKE_SRC_RSTBTN | M5PM1_WAKE_SRC_CMD_RST | M5PM1_WAKE_SRC_EXT_WAKE |
                                            M5PM1_WAKE_SRC_5VINOUT;

[[noreturn]] void fail(const char* stage, const char* step, esp_err_t error, int raw_error)
{
    ESP_LOGE(kTag, "TEST_FAIL stage=%s step=%s error=%s raw=%d", stage, step, esp_err_to_name(error), raw_error);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "imu_wake_shutdown_failed");
    m5pm::test_support::hold_safe_idle();
}

void finish_wake(const m5pm::power::WakeStatus& pm1, const m5pm::wakeup::ImuStatus& imu)
{
    const bool external_wake = (pm1.pm1_wake & M5PM1_WAKE_SRC_EXT_WAKE) != 0U;
    const bool unrelated_wake =
        (pm1.pm1_wake & static_cast<std::uint8_t>(kAllPm1WakeSources & ~M5PM1_WAKE_SRC_EXT_WAKE)) != 0U;
    const bool gpio4_triggered = (pm1.pm1_gpio_irq & M5PM1_IRQ_GPIO4) != 0U;
    const bool unrelated_gpio =
        (pm1.pm1_gpio_irq & static_cast<std::uint8_t>(M5PM1_IRQ_GPIO_ALL & ~M5PM1_IRQ_GPIO4)) != 0U;
    const bool target_evidence = external_wake || gpio4_triggered || imu.any_motion;
    const bool valid           = external_wake && !unrelated_wake && !unrelated_gpio && pm1.pm1_system_irq == 0U &&
                       pm1.pm1_button_irq == 0U && imu.any_motion;

    if (!target_evidence) {
        return;
    }

    ESP_LOGI(kTag,
             "WAKE_CAPTURE source=imu pm1=0x%02X gpio_irq=0x%02X "
             "sys_irq=0x%02X btn_irq=0x%02X imu=0x%04X any_motion=%d",
             pm1.pm1_wake, pm1.pm1_gpio_irq, pm1.pm1_system_irq, pm1.pm1_button_irq, imu.raw, imu.any_motion);
    if (valid) {
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, "imu_wake_shutdown");
    } else {
        ESP_LOGE(kTag,
                 "WAKE_FAIL expected_pm1_ext_wake=1 expected_imu_any_motion=1 "
                 "unrelated_events_must_be=0");
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "wake_source_mismatch");
    }
    m5pm::test_support::hold_safe_idle();
}
}  // namespace

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    ESP_ERROR_CHECK(m5pm::test_support::initialize(&context));

    const esp_err_t i2c_result = m5pm::board::initialize_i2c();
    if (i2c_result != ESP_OK) {
        fail("board", "i2c_initialize", i2c_result, i2c_result);
    }

    i2c_bus_handle_t bus         = nullptr;
    const esp_err_t claim_result = m5pm::board::acquire_i2c(&bus);
    if (claim_result != ESP_OK) {
        fail("board", "shared_bus", claim_result, claim_result);
    }

    m5pm::wakeup::ImuStatus retained_imu          = {};
    const m5pm::wakeup::ImuResult retained_result = m5pm::wakeup::ImuWakeup::read_retained_status(bus, &retained_imu);
    if (!retained_result.ok()) {
        fail("wake_capture", retained_result.step, retained_result.error, retained_result.raw_error);
    }

    m5pm::power::PowerController power;
    m5pm::power::PowerResult power_result = power.begin(bus);
    if (!power_result.ok()) {
        fail("board", "pm1_begin", power_result.error, power_result.raw_error);
    }

    m5pm::power::WakeStatus retained_pm1 = {};
    power_result                         = power.capture_wake_status(&retained_pm1);
    if (!power_result.ok()) {
        fail("wake_capture", power_result.step, power_result.error, power_result.raw_error);
    }
    finish_wake(retained_pm1, retained_imu);

    power_result = power.prepare_imu_wake_shutdown();
    if (!power_result.ok()) {
        fail("shutdown_prepare", power_result.step, power_result.error, power_result.raw_error);
    }

    m5pm::wakeup::ImuWakeup imu;
    m5pm::wakeup::ImuResult imu_result = imu.begin(bus);
    if (!imu_result.ok()) {
        fail("imu_arm", imu_result.step, imu_result.error, imu_result.raw_error);
    }
    imu_result = imu.configure_any_motion(kImuWakeProfile);
    if (!imu_result.ok()) {
        fail("imu_arm", imu_result.step, imu_result.error, imu_result.raw_error);
    }
    imu_result = imu.release_bus();
    if (!imu_result.ok()) {
        fail("imu_arm", imu_result.step, imu_result.error, imu_result.raw_error);
    }

    esp_log_level_set("M5PM1_GPIO", ESP_LOG_ERROR);
    esp_log_level_set("M5PM1_I2C", ESP_LOG_ERROR);
    ESP_LOGI(kTag, "BOARD_OK PM1=1 IMU=1 PROFILE=%s WAKE=imu", kImuWakeProfileName);
    ESP_LOGI(kTag, "WAKE_ARMED source=imu path=pm1_gpio4+shutdown");
    const esp_err_t ready_result = m5pm::test_support::mark_test_ready();
    if (ready_result != ESP_OK) {
        fail("test", "mark_ready", ready_result, ready_result);
    }
    vTaskDelay(kPrepareDelay);

    power_result = power.enter_imu_wake_shutdown();
    if (!power_result.ok()) {
        fail("shutdown_enter", power_result.step, power_result.error, power_result.raw_error);
    }

    // A successful shutdown request must not be followed by logging or I2C traffic.
    m5pm::test_support::hold_safe_idle();
}
