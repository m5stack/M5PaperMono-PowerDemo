#include "m5pm_wifi.hpp"

#include <cstdint>
#include <cstring>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "apps/ping/ping_sock.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

namespace m5pm::wifi {
namespace {

EventGroupHandle_t g_events  = nullptr;
constexpr EventBits_t kGotIp = BIT0;

esp_err_t ensure_nvs_initialized()
{
    esp_err_t error = nvs_flash_init();
    if (error == ESP_OK || error == ESP_ERR_INVALID_STATE) return ESP_OK;
    if (error != ESP_ERR_NVS_NO_FREE_PAGES && error != ESP_ERR_NVS_NEW_VERSION_FOUND) return error;

    error = nvs_flash_erase();
    if (error != ESP_OK) return error;
    return nvs_flash_init();
}

void event_handler(void*, esp_event_base_t base, std::int32_t id, void*)
{
    if (g_events == nullptr) return;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(g_events, kGotIp);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(g_events, kGotIp);
    }
}

}  // namespace

esp_err_t connect(PowerSave mode)
{
    if (CONFIG_M5PM_WIFI_SSID[0] == '\0') return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ensure_nvs_initialized(), "wifi", "nvs");
    ESP_RETURN_ON_ERROR(esp_netif_init(), "wifi", "netif");
    esp_err_t error = esp_event_loop_create_default();
    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) return error;
    if (esp_netif_create_default_wifi_sta() == nullptr) return ESP_FAIL;

    g_events = xEventGroupCreate();
    if (g_events == nullptr) return ESP_ERR_NO_MEM;
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), "wifi", "init");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, nullptr), "wifi",
                        "ip_event");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, event_handler, nullptr),
                        "wifi", "disconnect_event");

    wifi_config_t sta = {};
    std::strncpy(reinterpret_cast<char*>(sta.sta.ssid), CONFIG_M5PM_WIFI_SSID, sizeof(sta.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(sta.sta.password), CONFIG_M5PM_WIFI_PASSWORD, sizeof(sta.sta.password) - 1);
    sta.sta.scan_method = WIFI_FAST_SCAN;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), "wifi", "mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta), "wifi", "config");
    ESP_RETURN_ON_ERROR(esp_wifi_set_ps(mode == PowerSave::none ? WIFI_PS_NONE : WIFI_PS_MAX_MODEM), "wifi", "ps");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), "wifi", "start");
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), "wifi", "connect");

    const EventBits_t bits =
        xEventGroupWaitBits(g_events, kGotIp, pdFALSE, pdTRUE, pdMS_TO_TICKS(CONFIG_M5PM_WIFI_CONNECT_TIMEOUT_MS));
    return (bits & kGotIp) != 0 ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t start_fixed_ping()
{
    ip_addr_t target = {};
    if (ipaddr_aton(CONFIG_M5PM_WIFI_PING_TARGET, &target) == 0 || !IP_IS_V4(&target) || ip_addr_isany(&target)) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_ping_config_t config       = ESP_PING_DEFAULT_CONFIG();
    config.target_addr             = target;
    config.count                   = 0;
    config.interval_ms             = 1000;
    config.timeout_ms              = 500;
    config.data_size               = 64;
    esp_ping_callbacks_t callbacks = {};
    esp_ping_handle_t handle       = nullptr;
    const esp_err_t error          = esp_ping_new_session(&config, &callbacks, &handle);
    return error == ESP_OK ? esp_ping_start(handle) : error;
}

bool connected()
{
    return g_events != nullptr && (xEventGroupGetBits(g_events) & kGotIp) != 0;
}

}  // namespace m5pm::wifi
