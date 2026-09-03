#include "esp_log.h"
#include "esp_pm.h"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"
#include "m5pm_wifi.hpp"
#include "sdkconfig.h"

static_assert(CONFIG_PM_ENABLE == 1, "Wi-Fi Light Sleep requires dynamic power management");
static_assert(CONFIG_FREERTOS_USE_TICKLESS_IDLE == 1, "Wi-Fi Light Sleep requires tickless idle");

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) m5pm::phase_test::fail("test_support", ESP_FAIL);
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok())
        m5pm::phase_test::fail(baseline_result.step, baseline_result.error, baseline_result.raw_error);
    const esp_err_t wifi_error = m5pm::wifi::connect(m5pm::wifi::PowerSave::max_modem);
    if (wifi_error != ESP_OK) m5pm::phase_test::fail("wifi_connect", wifi_error);
    esp_pm_config_t requested    = {};
    requested.max_freq_mhz       = 240;
    requested.min_freq_mhz       = 40;
    requested.light_sleep_enable = true;
    esp_err_t error              = esp_pm_configure(&requested);
    if (error != ESP_OK) m5pm::phase_test::fail("pm_configure", error);
    esp_pm_config_t actual = {};
    error                  = esp_pm_get_configuration(&actual);
    if (error != ESP_OK) m5pm::phase_test::fail("pm_readback", error);
    if (actual.max_freq_mhz != 240 || actual.min_freq_mhz != 40 || !actual.light_sleep_enable) {
        m5pm::phase_test::fail("pm_verify", ESP_ERR_INVALID_RESPONSE);
    }
    ESP_LOGI("wifi_light_sleep",
             "BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=max_modem APP_TRAFFIC=0 EXPLICIT_SCAN=0 "
             "AUTO_LIGHT_SLEEP=1 TICKLESS=1 CPU_MIN_MHZ=40 CPU_MAX_MHZ=240");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    m5pm::phase_test::wait_measurement_window();
    if (!m5pm::wifi::connected()) m5pm::phase_test::fail("wifi_disconnected", ESP_ERR_INVALID_STATE);
    m5pm::phase_test::pass_and_hold();
}
