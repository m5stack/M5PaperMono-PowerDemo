#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "m5pm_board.hpp"
#include "m5pm_display.hpp"
#include "m5pm_lora.hpp"
#include "m5pm_phase_test.hpp"
#include "m5pm_test_support.hpp"
#include "M5UnitUnified.h"
#include "M5UnitUnifiedNFC.h"
#include "sdkconfig.h"

namespace {
constexpr char kTag[]                  = "full_load";
constexpr EventBits_t kWifiConnected   = BIT0;
constexpr std::uint32_t kWifiTimeoutMs = 8000;
m5pm::lora::Radio g_lora;
m5::unit::UnitUnified g_nfc_hub;
m5::unit::UnitNFC g_nfc_unit;
m5::nfc::NFCLayerA g_nfc_layer_a{g_nfc_unit};

[[noreturn]] void fail(const char* detail)
{
    ESP_LOGE(kTag, "TEST_FAIL detail=%s", detail);
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, detail);
    m5pm::test_support::hold_safe_idle();
}

void wifi_event(void* arg, esp_event_base_t base, int32_t id, void*)
{
    auto* events = static_cast<EventGroupHandle_t>(arg);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) (void)esp_wifi_connect();
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) xEventGroupSetBits(events, kWifiConnected);
}

bool touch_title_pressed(i2c_bus_device_handle_t touch)
{
    std::uint8_t points = 0;
    if (i2c_bus_read_byte(touch, 0x02, &points) != ESP_OK || points == 0U) return false;
    std::uint8_t coords[4] = {};
    if (i2c_bus_read_bytes(touch, 0x03, sizeof(coords), coords) != ESP_OK) return false;
    const std::uint16_t y = static_cast<std::uint16_t>(((coords[2] & 0x0FU) << 8U) | coords[3]);
    return y < 80U;
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
}  // namespace

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    ESP_ERROR_CHECK(m5pm::test_support::initialize(&context));
#if !CONFIG_M5PM_FULL_LOAD_ENABLE
    fail("safety_gate_disabled");
#endif
    if (CONFIG_M5PM_FULL_LOAD_WIFI_SSID[0] == '\0') fail("wifi_credentials_empty");
    esp_err_t nvs_error = nvs_flash_init();
    if (nvs_error == ESP_ERR_NVS_NO_FREE_PAGES || nvs_error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_error);

    // Establish the validated PaperMono awake power baseline before enabling
    // Wi-Fi, so the first association burst starts from a known rail state.
    static m5pm::phase_test::AwakeL2 baseline;
    const auto baseline_result = baseline.begin();
    if (!baseline_result.ok()) fail(baseline_result.step);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    EventGroupHandle_t events = xEventGroupCreate();
    if (events == nullptr) fail("event_group");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, events));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event, events));
    wifi_config_t station = {};
    std::strncpy(reinterpret_cast<char*>(station.sta.ssid), CONFIG_M5PM_FULL_LOAD_WIFI_SSID,
                 sizeof(station.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(station.sta.password), CONFIG_M5PM_FULL_LOAD_WIFI_PASSWORD,
                 sizeof(station.sta.password) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station));
    ESP_ERROR_CHECK(esp_wifi_start());
    const EventBits_t wifi_bits =
        xEventGroupWaitBits(events, kWifiConnected, pdFALSE, pdFALSE, pdMS_TO_TICKS(kWifiTimeoutMs));
    ESP_LOGI(kTag, "WIFI=%s timeout_ms=%u", (wifi_bits & kWifiConnected) != 0 ? "connected" : "timeout",
             static_cast<unsigned>(kWifiTimeoutMs));
    if ((wifi_bits & kWifiConnected) == 0) fail("wifi_connect_timeout");

    auto power_result = baseline.power().restore_display();
    if (!power_result.ok()) fail(power_result.step);
    static m5pm::display::Controller display;
    if (display.begin() != ESP_OK || display.render_white(m5gfx::epd_mode_t::epd_fastest) != ESP_OK) {
        fail("display_refresh");
    }
    power_result = baseline.power().set_frontlight(100);
    if (!power_result.ok()) fail(power_result.step);
    if (baseline.power().restore_lora().ok() == false || g_lora.begin() != ESP_OK) fail("lora_begin");
    if (m5pm::phase_test::configure_stop_buttons() != ESP_OK) fail("button_config");
    i2c_bus_device_handle_t touch = i2c_bus_device_create(baseline.bus(), m5pm::board::kTouchAddress, 100000);
    if (touch == nullptr) fail("touch_device");
    power_result = baseline.power().restore_nfc();
    if (!power_result.ok()) fail(power_result.step);
    if (!nfc_reader_begin(baseline.bus())) {
        (void)display.render_full_load_status(true, 0, false, 0, m5gfx::epd_mode_t::epd_fastest);
        fail("nfc_a_begin");
    }
    ESP_LOGI(kTag,
             "BOARD_OK FRONTLIGHT=100 DISPLAY_MODE=epd_fastest DISPLAY_ORIENTATION=portrait DISPLAY_AREA=480x800 "
             "LORA=868MHz/SF12/22dBm NFC=nfc_a_polling TOUCH=active");
    if (m5pm::test_support::mark_test_ready() != ESP_OK) fail("test_ready");
    if (display.render_full_load_status(true, 0, true, 0, m5gfx::epd_mode_t::epd_fastest) != ESP_OK) {
        fail("status_refresh");
    }
    ESP_LOGI(kTag, "LOAD_RUNNING STOP=GPIO2_OR_GPIO3_OR_TOUCH_TITLE HARD_TIMEOUT=%us",
             CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS);
    const TickType_t deadline    = xTaskGetTickCount() + pdMS_TO_TICKS(CONFIG_M5PM_FULL_LOAD_RUNTIME_SECONDS * 1000U);
    std::uint32_t tx_count       = 0;
    std::uint32_t nfc_detections = 0;
    while (xTaskGetTickCount() < deadline && !m5pm::phase_test::stop_requested() && !touch_title_pressed(touch)) {
        char payload[16] = {};
        std::snprintf(payload, sizeof(payload), "PM_%u", static_cast<unsigned>(++tx_count));
        if (g_lora.transmit(payload, 1000) != ESP_OK) fail("lora_transmit");
        g_nfc_hub.update();
        std::vector<m5::nfc::a::PICC> piccs;
        if (g_nfc_layer_a.detect(piccs) && !piccs.empty()) {
            nfc_detections += static_cast<std::uint32_t>(piccs.size());
            g_nfc_layer_a.deactivate();
        }
        if (display.render_full_load_status(true, tx_count, true, nfc_detections, m5gfx::epd_mode_t::epd_fastest) !=
            ESP_OK) {
            fail("status_refresh");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    (void)baseline.power().set_frontlight(50);
    const esp_err_t lora_stop_error = g_lora.stop();
    if (lora_stop_error != ESP_OK) fail("lora_stop");
    const auto lora_power_off_result = baseline.power().power_off_lora();
    if (!lora_power_off_result.ok()) fail(lora_power_off_result.step);
    (void)esp_wifi_stop();
    (void)esp_wifi_deinit();
    (void)i2c_bus_device_delete(&touch);
    g_nfc_unit.disableField();
    m5pm::test_support::report_final(m5pm::test_support::FinalStatus::pass, "full_load_stopped_and_cleaned");
    m5pm::test_support::hold_safe_idle();
}
