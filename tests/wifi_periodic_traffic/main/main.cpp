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
    esp_err_t error = m5pm::wifi::connect(m5pm::wifi::PowerSave::none);
    if (error != ESP_OK) m5pm::phase_test::fail("wifi_connect", error);
    error = m5pm::wifi::start_fixed_ping();
    if (error != ESP_OK) m5pm::phase_test::fail("icmp_start", error);
    ESP_LOGI("wifi_periodic_traffic",
             "BOARD_OK BASELINE=l2-awake WIFI=connected MODE=sta PS=none ICMP_PAYLOAD_BYTES=64 PERIOD_MS=1000 "
             "TARGET=fixed_lan ACK_POLICY=none RETRY_WORKLOAD=0 EXPLICIT_SCAN=0");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    m5pm::phase_test::wait_measurement_window();
    if (!m5pm::wifi::connected()) m5pm::phase_test::fail("wifi_disconnected", ESP_ERR_INVALID_STATE);
    m5pm::phase_test::pass_and_hold();
}
