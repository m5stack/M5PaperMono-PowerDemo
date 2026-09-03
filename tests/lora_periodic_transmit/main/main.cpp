#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "m5pm_lora.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr std::int64_t kRuntimeLimitUs = 90000000;
constexpr std::uint32_t kTxTimeoutMs   = 10000;
}  // namespace

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
    if (m5pm::phase_test::configure_stop_buttons() != ESP_OK) m5pm::phase_test::fail("stop_buttons", ESP_FAIL);
    static m5pm::lora::Radio radio;
    if (radio.begin() != ESP_OK) m5pm::phase_test::fail("lora_begin", ESP_FAIL);
    ESP_LOGI("lora_periodic_transmit",
             "BOARD_OK BASELINE=l2-awake RADIO=sx1262 FREQ_MHZ=868 BW_KHZ=125 SF=12 CR=4/5 SYNC=0x34 TX_DBM=22 "
             "PREAMBLE=8 TCXO_V=3.0 REGULATOR=ldo PAYLOAD=PM_n PERIOD_AFTER_TX_MS=1000 ACK=0 RETRY=0 "
             "LOCAL_TX_DONE=required RUNTIME_LIMIT_MS=90000 STOP=key1_or_key2");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) m5pm::phase_test::fail("test_ready", ESP_FAIL);

    const std::int64_t start = esp_timer_get_time();
    std::uint32_t sequence   = 0;
    bool manual_stop         = false;
    while (esp_timer_get_time() - start < kRuntimeLimitUs) {
        if (m5pm::phase_test::stop_requested()) {
            manual_stop = true;
            break;
        }
        char payload[32] = {};
        std::snprintf(payload, sizeof(payload), "PM_%lu", static_cast<unsigned long>(sequence++));
        const std::int64_t remaining_us   = kRuntimeLimitUs - (esp_timer_get_time() - start);
        const std::uint32_t remaining_ms  = static_cast<std::uint32_t>((remaining_us + 999) / 1000);
        const std::uint32_t tx_timeout_ms = remaining_ms < kTxTimeoutMs ? remaining_ms : kTxTimeoutMs;
        const esp_err_t tx_error          = radio.transmit(payload, tx_timeout_ms);
        if (tx_error != ESP_OK) {
            if (esp_timer_get_time() - start >= kRuntimeLimitUs) break;
            m5pm::phase_test::fail("local_tx_done", tx_error);
        }
        const TickType_t wait_start = xTaskGetTickCount();
        while (xTaskGetTickCount() - wait_start < pdMS_TO_TICKS(1000)) {
            if (m5pm::phase_test::stop_requested()) {
                manual_stop = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        if (manual_stop) break;
    }
    if (radio.stop() != ESP_OK) m5pm::phase_test::fail("lora_stop", ESP_FAIL);
    power_result = baseline.power().power_off_lora();
    if (!power_result.ok()) m5pm::phase_test::fail(power_result.step, power_result.error, power_result.raw_error);
    m5pm::phase_test::pass_and_hold(manual_stop ? "manual_stop" : "runtime_limit_complete");
}
