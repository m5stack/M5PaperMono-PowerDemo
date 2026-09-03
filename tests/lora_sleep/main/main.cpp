#include "esp_log.h"
#include "m5pm_lora.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    if (m5pm::test_support::initialize(&context) != ESP_OK) m5pm::phase_test::fail("test_support", ESP_FAIL);
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok())
        m5pm::phase_test::fail(baseline_result.step, baseline_result.error, baseline_result.raw_error);
    auto power_result = baseline.power().restore_lora();
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    static m5pm::lora::Radio radio;
    if (radio.begin() != ESP_OK) m5pm::phase_test::fail("lora_begin", ESP_FAIL);
    if (radio.sleep() != ESP_OK) m5pm::phase_test::fail("lora_sleep", ESP_FAIL);
    ESP_LOGI("lora_sleep",
             "BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 "
             "PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo STATE=sleep TX=0 RX=0");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);
    m5pm::phase_test::wait_measurement_window(90000);
    m5pm::phase_test::pass_and_hold();
}
