#include <cstdint>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_display.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr char kTag[]                       = "normal_active";
constexpr std::int64_t kMeasurementWindowUs = 60'000'000;
constexpr std::int64_t kRefreshPeriodUs     = 5'000'000;

[[noreturn]] void fail(const char* step, esp_err_t error = ESP_FAIL)
{
    ESP_LOGE(kTag, "BOARD_FAIL stage=%s error=%s", step, esp_err_to_name(error));
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, step);
    m5pm::test_support::hold_safe_idle();
}
}  // namespace

extern "C" {
extern const std::uint8_t img_logo_start[] asm("_binary_img_logo_png_start");
extern const std::uint8_t img_logo_end[] asm("_binary_img_logo_png_end");
}

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) fail("test_support");

    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok()) fail(baseline_result.step, baseline_result.raw_error);

    auto power_result = baseline.power().restore_display();
    if (!power_result.ok()) fail(power_result.step, power_result.raw_error);

    static m5pm::display::Controller display;
    if (display.begin() != ESP_OK) fail("display_begin");
    std::uint32_t refresh_count = 1;
    if (display.render_png_counter(img_logo_start, static_cast<std::size_t>(img_logo_end - img_logo_start),
                                   refresh_count, m5gfx::epd_mode_t::epd_quality) != ESP_OK) {
        fail("initial_refresh");
    }
    power_result = baseline.power().set_frontlight(50);
    if (!power_result.ok()) fail(power_result.step, power_result.raw_error);

    ESP_LOGI(kTag,
             "BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality FULL_SCREEN=1 "
             "REFRESH_PERIOD_MS=5000 "
             "REFRESH_COUNT_INITIAL=1 AREA=480x800 FRONTLIGHT_PERCENT=50 "
             "WIFI=off NFC=off LORA=off WORKLOAD=idle-frame-loop");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) fail("test_ready");

    const std::int64_t ready_time  = esp_timer_get_time();
    std::int64_t next_refresh_time = ready_time + kRefreshPeriodUs;
    bool pass_reported             = false;
    for (;;) {
        taskYIELD();
        const std::int64_t now = esp_timer_get_time();
        if (now >= next_refresh_time) {
            ++refresh_count;
            if (display.render_png_counter(img_logo_start, static_cast<std::size_t>(img_logo_end - img_logo_start),
                                           refresh_count, m5gfx::epd_mode_t::epd_quality) != ESP_OK) {
                fail("periodic_refresh");
            }
            next_refresh_time = esp_timer_get_time() + kRefreshPeriodUs;
        }
        if (!pass_reported && now - ready_time >= kMeasurementWindowUs) {
            m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, "measurement_window_complete");
            pass_reported = true;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
