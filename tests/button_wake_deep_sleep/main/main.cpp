#include <cstdint>

#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"
#include "m5pm_imu_wakeup.hpp"
#include "m5pm_power.hpp"
#include "m5pm_rtc_wakeup.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr char kTag[]               = "button_wake_deep";
constexpr TickType_t kPrepareDelay  = pdMS_TO_TICKS(3000);
constexpr std::uint64_t kButtonMask = (1ULL << m5pm::board::kKey1) | (1ULL << m5pm::board::kKey2);

esp_err_t configure_buttons()
{
    gpio_config_t config = {};
    config.intr_type     = GPIO_INTR_DISABLE;
    config.mode          = GPIO_MODE_INPUT;
    config.pin_bit_mask  = kButtonMask;
    config.pull_up_en    = GPIO_PULLUP_ENABLE;
    config.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    return gpio_config(&config);
}

[[noreturn]] void fail(const char* stage, const char* step, esp_err_t error, int raw_error)
{
    ESP_LOGE(kTag, "TEST_FAIL stage=%s step=%s error=%s raw=%d", stage, step, esp_err_to_name(error), raw_error);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "button_wake_deep_sleep_failed");
    m5pm::test_support::hold_safe_idle();
}

[[noreturn]] void finish_wake(const m5pm::test_support::TestContext& context, std::uint64_t ext1_status,
                              const m5pm::power::WakeStatus& pm1, const m5pm::wakeup::RtcFlags& rtc,
                              const m5pm::wakeup::ImuStatus& imu)
{
    const bool key1              = (ext1_status & (1ULL << m5pm::board::kKey1)) != 0U;
    const bool key2              = (ext1_status & (1ULL << m5pm::board::kKey2)) != 0U;
    const bool unexpected_button = (ext1_status & ~kButtonMask) != 0U;
    const bool rtc_triggered     = rtc.alarm() || rtc.timer();
    const bool valid = context.wake_cause == ESP_SLEEP_WAKEUP_EXT1 && (key1 || key2) && !unexpected_button &&
                       pm1.pm1_wake == 0U && pm1.pm1_gpio_irq == 0U && pm1.pm1_system_irq == 0U &&
                       pm1.pm1_button_irq == 0U && !rtc_triggered && !imu.any_motion;

    ESP_LOGI(kTag,
             "WAKE_CAPTURE source=button esp=%s ext1=0x%016llX key1=%d key2=%d "
             "pm1=0x%02X gpio_irq=0x%02X sys_irq=0x%02X btn_irq=0x%02X rtc=0x%02X imu=0x%04X",
             m5pm::test_support::wake_cause_name(context.wake_cause), static_cast<unsigned long long>(ext1_status),
             key1, key2, pm1.pm1_wake, pm1.pm1_gpio_irq, pm1.pm1_system_irq, pm1.pm1_button_irq, rtc.raw, imu.raw);
    if (valid) {
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, "button_wake_deep_sleep");
    } else {
        ESP_LOGE(kTag,
                 "WAKE_FAIL expected_esp=ext1 expected_button_mask=0x%016llX "
                 "rtc_alarm_timer_imu_pm1_events_must_be=0",
                 static_cast<unsigned long long>(kButtonMask));
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

    const esp_err_t button_result = configure_buttons();
    if (button_result != ESP_OK) {
        fail("board", "button_gpio_config", button_result, button_result);
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

    m5pm::wakeup::RtcWakeup rtc;
    m5pm::wakeup::RtcResult rtc_result = rtc.begin(bus);
    if (!rtc_result.ok()) {
        fail("board", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    }
    m5pm::wakeup::RtcFlags retained_rtc = {};
    rtc_result                          = rtc.read_flags(&retained_rtc);
    if (!rtc_result.ok()) {
        fail("wake_capture", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    }

    m5pm::power::PowerController power;
    m5pm::power::PowerResult power_result = power.begin_l2(bus);
    if (!power_result.ok()) {
        fail("board", "l2_begin", power_result.error, power_result.raw_error);
    }
    m5pm::power::WakeStatus retained_pm1 = {};
    power_result                         = power.capture_wake_status(&retained_pm1);
    if (!power_result.ok()) {
        fail("wake_capture", power_result.step, power_result.error, power_result.raw_error);
    }

    if (context.wake_cause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        finish_wake(context, esp_sleep_get_ext1_wakeup_status(), retained_pm1, retained_rtc, retained_imu);
    }

    rtc_result = rtc.configure_button_baseline();
    if (!rtc_result.ok()) {
        fail("rtc_prepare", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    }
    rtc_result = rtc.release_bus();
    if (!rtc_result.ok()) {
        fail("rtc_prepare", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    }

    esp_log_level_set("M5IOE1_I2C", ESP_LOG_ERROR);
    esp_log_level_set("M5IOE1_SYS", ESP_LOG_ERROR);
    esp_log_level_set("M5PM1_GPIO", ESP_LOG_ERROR);
    esp_log_level_set("M5PM1_I2C", ESP_LOG_ERROR);
    ESP_LOGI(kTag, "BOARD_OK PM1=1 IOE1=1 RTC=1 WAKE=button");
    ESP_LOGI(kTag, "WAKE_ARMED source=button path=esp_ext1 gpio_mask=0x%016llX",
             static_cast<unsigned long long>(kButtonMask));
    const esp_err_t ready_result = m5pm::test_support::mark_test_ready();
    if (ready_result != ESP_OK) {
        fail("test", "mark_ready", ready_result, ready_result);
    }
    vTaskDelay(kPrepareDelay);

    power_result = power.enter_button_wake_l2();
    fail("l2_enter", power_result.step, power_result.error, power_result.raw_error);
}
