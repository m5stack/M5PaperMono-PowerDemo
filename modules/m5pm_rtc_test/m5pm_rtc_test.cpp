#include "m5pm_rtc_test.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"
#include "m5pm_power.hpp"
#include "m5pm_rtc_wakeup.hpp"
#include "m5pm_test_support.hpp"

namespace m5pm::rtc_test {
namespace {
constexpr TickType_t kPrepareDelay = pdMS_TO_TICKS(3000);

[[noreturn]] void fail(const char* tag, const char* test_id, const char* stage, const char* step, esp_err_t error,
                       int raw)
{
    ESP_LOGE(tag, "TEST_FAIL stage=%s step=%s error=%s raw=%d", stage, step, esp_err_to_name(error), raw);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, test_id);
    m5pm::test_support::hold_safe_idle();
}
}  // namespace

void run(const char* tag, const char* test_id, Event event, bool shutdown)
{
    m5pm::test_support::TestContext context = {};
    ESP_ERROR_CHECK(m5pm::test_support::initialize(&context));
    const esp_err_t i2c_result = m5pm::board::initialize_i2c();
    if (i2c_result != ESP_OK) fail(tag, test_id, "board", "i2c_initialize", i2c_result, i2c_result);
    i2c_bus_handle_t bus         = nullptr;
    const esp_err_t claim_result = m5pm::board::acquire_i2c(&bus);
    if (claim_result != ESP_OK) fail(tag, test_id, "board", "shared_bus", claim_result, claim_result);

    m5pm::wakeup::RtcWakeup rtc;
    auto rtc_result = rtc.begin(bus);
    if (!rtc_result.ok()) fail(tag, test_id, "board", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    m5pm::wakeup::RtcFlags retained = {};
    rtc_result                      = rtc.read_flags(&retained);
    if (!rtc_result.ok()) fail(tag, test_id, "wake_capture", rtc_result.step, rtc_result.error, rtc_result.raw_error);

    m5pm::power::PowerController power;
    m5pm::power::PowerResult power_result = shutdown ? power.begin(bus) : power.begin_l2(bus);
    if (!power_result.ok()) fail(tag, test_id, "board", "power_begin", power_result.error, power_result.raw_error);
    m5pm::power::WakeStatus pm1 = {};
    power_result                = power.capture_wake_status(&pm1);
    if (!power_result.ok())
        fail(tag, test_id, "wake_capture", power_result.step, power_result.error, power_result.raw_error);

    const bool target_flag       = event == Event::alarm ? retained.alarm() : retained.timer();
    const bool other_flag        = event == Event::alarm ? retained.timer() : retained.alarm();
    const bool pm1_external_wake = (pm1.pm1_wake & M5PM1_WAKE_SRC_EXT_WAKE) != 0U;
    const bool pm1_gpio0         = (pm1.pm1_gpio_irq & M5PM1_IRQ_GPIO0) != 0U;
    // PM1 shutdown restarts the ESP32 as a normal reset, so wake_cause is undefined.
    // RTC/PM1 evidence is the discriminator for a shutdown wake; Deep Sleep keeps
    // using the ESP wake cause to avoid treating a normal boot as a wake event.
    // RTC flags survive an ESP32 reset and may be stale after a brownout or a
    // USB power cycle. For PM1 shutdown, only PM1 external-wake evidence can
    // prove that the previous cycle actually entered and exited shutdown.
    const bool wake_candidate = shutdown ? pm1_external_wake : (context.wake_cause != ESP_SLEEP_WAKEUP_UNDEFINED);
    if (wake_candidate) {
        // RX8130 may retain a calendar alarm flag even while its interrupt is
        // masked. Timer wake is accepted when its timer event and wake path
        // are present; alarm wake still rejects a competing timer event.
        const bool valid = shutdown ? (target_flag && (event == Event::timer || !other_flag) && pm1_external_wake)
                                    : (context.wake_cause == ESP_SLEEP_WAKEUP_EXT0 && target_flag &&
                                       (event == Event::timer || !other_flag) && pm1_gpio0);
        ESP_LOGI(tag, "WAKE_CAPTURE source=rtc_%s esp=%s pm1=0x%02X gpio_irq=0x%02X rtc=0x%02X",
                 event == Event::alarm ? "alarm" : "timer", m5pm::test_support::wake_cause_name(context.wake_cause),
                 pm1.pm1_wake, pm1.pm1_gpio_irq, retained.raw);
        if (valid) {
            m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, test_id);
        } else {
            ESP_LOGE(tag, "WAKE_FAIL expected_rtc=%s expected_pm1_ext_wake=1",
                     event == Event::alarm ? "alarm" : "timer");
            m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "wake_source_mismatch");
        }
        m5pm::test_support::hold_safe_idle();
    }

    power_result = shutdown ? power.prepare_rtc_wake_shutdown()
                            : (event == Event::alarm ? power.prepare_rtc_alarm_wake_l2() : power.prepare_rtc_wake_l2());
    if (!power_result.ok())
        fail(tag, test_id, "power_prepare", power_result.step, power_result.error, power_result.raw_error);
    // Start the RTC countdown only after the complete low-power and wake path
    // is configured, so the requested interval starts at test readiness.
    rtc_result = event == Event::alarm ? rtc.configure_alarm_120s() : rtc.configure_timer_120s();
    if (!rtc_result.ok()) fail(tag, test_id, "rtc_prepare", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    rtc_result = rtc.release_bus();
    if (!rtc_result.ok()) fail(tag, test_id, "rtc_prepare", rtc_result.step, rtc_result.error, rtc_result.raw_error);
    ESP_LOGI(tag, "BOARD_OK PM1=1 IOE1=%d RTC=1 WAKE=rtc_%s", shutdown ? 0 : 1,
             event == Event::alarm ? "alarm" : "timer");
    ESP_LOGI(tag, "WAKE_ARMED source=rtc_%s interval=120s path=%s", event == Event::alarm ? "alarm" : "timer",
             shutdown ? "pm1_gpio0+shutdown" : "pm1_gpio0+aggregate_gpio1+esp_ext0");
    const esp_err_t ready = m5pm::test_support::mark_test_ready();
    if (ready != ESP_OK) fail(tag, test_id, "test", "mark_ready", ready, ready);
    vTaskDelay(kPrepareDelay);
    power_result = shutdown ? power.enter_rtc_wake_shutdown()
                            : (event == Event::alarm ? power.enter_rtc_alarm_wake_l2() : power.enter_rtc_wake_l2());
    if (!power_result.ok()) {
        fail(tag, test_id, "wake_enter", power_result.step, power_result.error, power_result.raw_error);
    }
    if (shutdown) {
        // A successful PM1 shutdown does not return on hardware. Keep the fallback
        // path quiet if the driver returns after requesting shutdown.
        m5pm::test_support::hold_safe_idle();
    }
    fail(tag, test_id, "wake_enter", power_result.step, power_result.error, power_result.raw_error);
}

}  // namespace m5pm::rtc_test
