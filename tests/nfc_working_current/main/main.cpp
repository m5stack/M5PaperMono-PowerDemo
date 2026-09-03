#include <cstdint>
#include <vector>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"
#include "M5UnitUnified.h"
#include "M5UnitUnifiedNFC.h"
#include "sdkconfig.h"

namespace {
constexpr char kTag[]                  = "nfc_working_current";
constexpr std::uint32_t kPollTimeoutMs = 100;
constexpr std::uint32_t kPollGapMs     = 20;

#if CONFIG_M5PM_NFC_MODE_POWER_DOWN
constexpr char kConfigId[] = "nfc-power-down";
#elif CONFIG_M5PM_NFC_MODE_NFC_A_POLLING
constexpr char kConfigId[] = "nfc-a-polling";
#else
#error "Select one NFC operating mode"
#endif

m5::unit::UnitUnified g_nfc_hub;
m5::unit::UnitNFC g_nfc_unit;
m5::nfc::NFCLayerA g_nfc_layer_a{g_nfc_unit};

[[noreturn]] void fail(const char* detail)
{
    ESP_LOGE(kTag, "TEST_FAIL detail=%s", detail);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, detail);
    m5pm::test_support::hold_safe_idle();
}

bool nfc_reader_begin(i2c_bus_handle_t bus)
{
    auto* native_bus = i2c_bus_get_internal_bus_handle(bus);
    if (native_bus == nullptr || !g_nfc_hub.add(g_nfc_unit, native_bus)) return false;

    auto config      = g_nfc_unit.config();
    config.mode      = m5::nfc::NFC::A;
    config.emulation = false;
    config.using_irq = false;
    g_nfc_unit.config(config);
    if (!g_nfc_hub.begin()) return false;
    g_nfc_hub.update();
    return g_nfc_unit.isNFCMode(m5::nfc::NFC::A);
}

bool enter_power_down(std::uint8_t* operation_control)
{
    if (operation_control == nullptr || !g_nfc_unit.disableField()) return false;
    vTaskDelay(pdMS_TO_TICKS(10));

    constexpr std::uint8_t kStopAllActivities = m5::unit::st25r3916::command::CMD_STOP_ALL_ACTIVITIES;
    if (!g_nfc_unit.writeDirectCommand(kStopAllActivities, &kStopAllActivities, 1)) return false;
    vTaskDelay(pdMS_TO_TICKS(10));

    if (!g_nfc_unit.writeOperationControl(0x00)) return false;
    return g_nfc_unit.readOperationControl(*operation_control) && *operation_control == 0x00;
}
}  // namespace

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    ESP_ERROR_CHECK(m5pm::test_support::initialize(&context, kConfigId));

    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok()) fail(baseline_result.step);
    const auto power_result = baseline.power().restore_nfc();
    if (!power_result.ok()) fail(power_result.step);
    ESP_LOGI(kTag, "NFC_POWER_READY settle_ms=120");
    if (!nfc_reader_begin(baseline.bus())) fail("nfc_a_begin");

#if CONFIG_M5PM_NFC_MODE_POWER_DOWN
    std::uint8_t operation_control = 0xFF;
    if (!enter_power_down(&operation_control)) fail("nfc_power_down");
    ESP_LOGI(kTag, "BOARD_OK NFC=power_down operation_control=0x%02X", operation_control);
#elif CONFIG_M5PM_NFC_MODE_NFC_A_POLLING
    g_nfc_hub.update();
    std::vector<m5::nfc::a::PICC> initial_piccs;
    if (g_nfc_layer_a.detect(initial_piccs, kPollTimeoutMs) && !initial_piccs.empty()) {
        (void)g_nfc_unit.disableField();
        fail("nfc_card_present");
    }
    ESP_LOGI(kTag, "BOARD_OK NFC=nfc_a_polling poll_period_ms=%u card=absent",
             static_cast<unsigned>(kPollTimeoutMs + kPollGapMs));
#endif
    if (m5pm::test_support::mark_test_ready() != ESP_OK) fail("test_ready");
#if CONFIG_M5PM_NFC_MODE_NFC_A_POLLING
    for (;;) {
        g_nfc_hub.update();
        std::vector<m5::nfc::a::PICC> piccs;
        if (g_nfc_layer_a.detect(piccs, kPollTimeoutMs) && !piccs.empty()) {
            (void)g_nfc_unit.disableField();
            fail("nfc_card_present");
        }
        vTaskDelay(pdMS_TO_TICKS(kPollGapMs));
    }
#elif CONFIG_M5PM_NFC_MODE_POWER_DOWN
    m5pm::test_support::hold_safe_idle();
#endif
}
