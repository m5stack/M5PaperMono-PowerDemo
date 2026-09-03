#include "esp_log.h"
#include "m5pm_display.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"
#include "sdkconfig.h"

namespace {
constexpr char kTag[] = "display_refresh_modes";
#if CONFIG_M5PM_DISPLAY_EPD_FASTEST
constexpr auto kMode       = m5gfx::epd_mode_t::epd_fastest;
constexpr char kModeName[] = "fastest";
#elif CONFIG_M5PM_DISPLAY_EPD_FAST
constexpr auto kMode       = m5gfx::epd_mode_t::epd_fast;
constexpr char kModeName[] = "fast";
#else
constexpr auto kMode       = m5gfx::epd_mode_t::epd_quality;
constexpr char kModeName[] = "quality";
#endif
}  // namespace

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
    if (display.render_white(kMode) != ESP_OK) {
        m5pm::phase_test::fail("initial_refresh", ESP_FAIL);
    }
    std::uint32_t refresh_count = 0;
    power_result                = baseline.power().set_frontlight(100);
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    ESP_LOGI(kTag,
             "BOARD_OK BASELINE=l2-awake DISPLAY=ssd1677 ORIENTATION=portrait REFRESH=%s AREA=480x800 PERIOD_MS=60000 "
             "FRONTLIGHT_PERCENT=100",
             kModeName);
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    bool pass_reported = false;
    bool show_logo     = false;
    for (;;) {
        m5pm::phase_test::wait_measurement_window();
        show_logo = !show_logo;
        if (show_logo) {
            ++refresh_count;
            ESP_LOGI(kTag, "frame_select type=logo counter=%u", static_cast<unsigned>(refresh_count));
            if (display.render_png_counter(img_logo_start, static_cast<std::size_t>(img_logo_end - img_logo_start),
                                           refresh_count, kMode) != ESP_OK) {
                m5pm::phase_test::fail("periodic_logo_refresh", ESP_FAIL);
            }
        } else {
            ESP_LOGI(kTag, "frame_select type=white");
            if (display.render_white(kMode) != ESP_OK) {
                m5pm::phase_test::fail("periodic_white_refresh", ESP_FAIL);
            }
        }
        if (!pass_reported) {
            m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, "first_periodic_refresh_complete");
            pass_reported = true;
        }
    }
}
