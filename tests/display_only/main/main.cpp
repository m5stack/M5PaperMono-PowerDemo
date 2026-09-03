#include "esp_log.h"
#include "m5pm_display.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr char kTag[] = "display_only";
}

extern "C" {
extern const std::uint8_t img_logo_start[] asm("_binary_img_logo_png_start");
extern const std::uint8_t img_logo_end[] asm("_binary_img_logo_png_end");
}

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) m5pm::phase_test::fail("test_support", ESP_FAIL);
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok())
        m5pm::phase_test::fail(baseline_result.step, baseline_result.error, baseline_result.raw_error);
    auto power_result = baseline.power().restore_display();
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    static m5pm::display::Controller display;
    if (display.begin() != ESP_OK) m5pm::phase_test::fail("display_begin", ESP_FAIL);
    if (display.render_png(img_logo_start, static_cast<std::size_t>(img_logo_end - img_logo_start),
                           m5gfx::epd_mode_t::epd_quality) != ESP_OK) {
        m5pm::phase_test::fail("display_refresh", ESP_FAIL);
    }
    power_result = baseline.power().set_frontlight(100);
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    ESP_LOGI(kTag,
             "BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=quality AREA=480x800 "
             "FRONTLIGHT_PERCENT=100");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    m5pm::phase_test::wait_measurement_window();
    m5pm::phase_test::pass_and_hold();
}
