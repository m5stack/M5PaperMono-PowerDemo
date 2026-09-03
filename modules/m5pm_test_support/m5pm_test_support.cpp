#include "m5pm_test_support.hpp"

#include <cstdio>
#include <inttypes.h>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace m5pm::test_support {
namespace {

constexpr char kTag[] = "m5pm_test";

bool g_initialized        = false;
bool g_test_ready         = false;
portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;

std::uint32_t fnv1a(const char* text)
{
    std::uint32_t hash = 2166136261u;
    while (*text != '\0') {
        hash ^= static_cast<std::uint8_t>(*text++);
        hash *= 16777619u;
    }
    return hash;
}

}  // namespace

esp_err_t initialize(TestContext* context)
{
    return initialize(context, CONFIG_M5PM_CONFIG_ID);
}

esp_err_t initialize(TestContext* context, const char* config_id)
{
    if (context == nullptr || config_id == nullptr || config_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&g_state_lock);
    if (g_initialized) {
        portEXIT_CRITICAL(&g_state_lock);
        return ESP_ERR_INVALID_STATE;
    }
    g_initialized = true;
    g_test_ready  = false;
    portEXIT_CRITICAL(&g_state_lock);

    const esp_app_desc_t* app = esp_app_get_description();
    context->test_id          = CONFIG_M5PM_TEST_ID;
    context->firmware_version = app->version;
    context->config_hash      = fnv1a(config_id);
    context->reset_reason     = esp_reset_reason();
    context->wake_cause       = esp_sleep_get_wakeup_cause();

    ESP_LOGI(kTag,
             "TEST_START test_id=%s firmware_version=%s config_id=%s "
             "config_hash=%08" PRIx32 " reset_reason=%s wake_cause=%s",
             context->test_id, context->firmware_version, config_id, context->config_hash,
             reset_reason_name(context->reset_reason), wake_cause_name(context->wake_cause));
    return ESP_OK;
}

esp_err_t mark_test_ready()
{
    portENTER_CRITICAL(&g_state_lock);
    if (!g_initialized || g_test_ready) {
        portEXIT_CRITICAL(&g_state_lock);
        return ESP_ERR_INVALID_STATE;
    }
    g_test_ready = true;
    portEXIT_CRITICAL(&g_state_lock);

    ESP_LOGI(kTag, "TEST_READY");
    std::fflush(stdout);
    return ESP_OK;
}

void report_final(FinalStatus status, const char* detail)
{
    const char* result = status == FinalStatus::pass ? "TEST_PASS" : "TEST_FAIL";
    if (detail == nullptr || detail[0] == '\0') {
        ESP_LOGI(kTag, "%s", result);
        return;
    }
    ESP_LOGI(kTag, "%s detail=%s", result, detail);
}

void hold_safe_idle()
{
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

bool diagnostics_enabled()
{
    portENTER_CRITICAL(&g_state_lock);
    const bool enabled = g_initialized && !g_test_ready;
    portEXIT_CRITICAL(&g_state_lock);
    return enabled;
}

bool test_ready()
{
    portENTER_CRITICAL(&g_state_lock);
    const bool ready = g_test_ready;
    portEXIT_CRITICAL(&g_state_lock);
    return ready;
}

const char* reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_POWERON:
            return "power_on";
        case ESP_RST_EXT:
            return "external_pin";
        case ESP_RST_SW:
            return "software";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt_watchdog";
        case ESP_RST_TASK_WDT:
            return "task_watchdog";
        case ESP_RST_WDT:
            return "watchdog";
        case ESP_RST_DEEPSLEEP:
            return "deep_sleep";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_SDIO:
            return "sdio";
        case ESP_RST_USB:
            return "usb";
        case ESP_RST_JTAG:
            return "jtag";
        case ESP_RST_EFUSE:
            return "efuse";
        case ESP_RST_PWR_GLITCH:
            return "power_glitch";
        case ESP_RST_CPU_LOCKUP:
            return "cpu_lockup";
        case ESP_RST_UNKNOWN:
        default:
            return "unknown";
    }
}

const char* wake_cause_name(esp_sleep_wakeup_cause_t cause)
{
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return "ext0";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "ext1";
        case ESP_SLEEP_WAKEUP_TIMER:
            return "timer";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "touchpad";
        case ESP_SLEEP_WAKEUP_ULP:
            return "ulp";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "gpio";
        case ESP_SLEEP_WAKEUP_UART:
            return "uart";
        case ESP_SLEEP_WAKEUP_WIFI:
            return "wifi";
        case ESP_SLEEP_WAKEUP_COCPU:
            return "coprocessor";
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            return "coprocessor_trap";
        case ESP_SLEEP_WAKEUP_BT:
            return "bluetooth";
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            return "undefined";
    }
}

}  // namespace m5pm::test_support
