#include "esp_log.h"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"
#include "m5pm_wifi.hpp"

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) m5pm::phase_test::fail("test_support", ESP_FAIL);
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok())
        m5pm::phase_test::fail(baseline_result.step, baseline_result.error, baseline_result.raw_error);
    const esp_err_t wifi_error = m5pm::wifi::connect(m5pm::wifi::PowerSave::none);
    if (wifi_error != ESP_OK) m5pm::phase_test::fail("wifi_connect", wifi_error);
    ESP_LOGI("wifi_connected",
             "BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none APP_TRAFFIC=0 EXPLICIT_SCAN=0");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    m5pm::phase_test::wait_measurement_window();
    if (!m5pm::wifi::connected()) m5pm::phase_test::fail("wifi_disconnected", ESP_ERR_INVALID_STATE);
    m5pm::phase_test::pass_and_hold();
}
