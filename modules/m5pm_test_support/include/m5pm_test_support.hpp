#pragma once

#include <cstdint>

#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_system.h"

namespace m5pm::test_support {

enum class FinalStatus : std::uint8_t {
    pass,
    fail,
};

struct TestContext {
    const char* test_id                 = nullptr;
    const char* firmware_version        = nullptr;
    std::uint32_t config_hash           = 0;
    esp_reset_reason_t reset_reason     = ESP_RST_UNKNOWN;
    esp_sleep_wakeup_cause_t wake_cause = ESP_SLEEP_WAKEUP_UNDEFINED;
};

esp_err_t initialize(TestContext* context);
esp_err_t initialize(TestContext* context, const char* config_id);
esp_err_t mark_test_ready();
void report_final(FinalStatus status, const char* detail = nullptr);
[[noreturn]] void hold_safe_idle();

[[nodiscard]] bool diagnostics_enabled();
[[nodiscard]] bool test_ready();
[[nodiscard]] const char* reset_reason_name(esp_reset_reason_t reason);
[[nodiscard]] const char* wake_cause_name(esp_sleep_wakeup_cause_t cause);

}  // namespace m5pm::test_support
