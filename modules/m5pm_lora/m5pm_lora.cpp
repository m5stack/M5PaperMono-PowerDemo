#include "m5pm_lora.hpp"

#include <RadioLib.h>
#include "hal/ESP-IDF/EspHal.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace m5pm::lora {
namespace {

EspHal g_hal(39, 40, 38, SPI3_HOST, 8000000);
Module g_module(&g_hal, 41, 5, RADIOLIB_NC, 21);
SX1262 g_radio(&g_module);
volatile bool g_tx_done = false;
bool g_ready            = false;

void on_packet_sent()
{
    g_tx_done = true;
}

esp_err_t radio_result(std::int16_t state)
{
    return state == RADIOLIB_ERR_NONE ? ESP_OK : ESP_FAIL;
}

}  // namespace

esp_err_t Radio::begin()
{
    const std::int16_t state = g_radio.begin(868.0, 125.0, 12, 5, 0x34, 22, 8, 3.0f, true);
    if (state != RADIOLIB_ERR_NONE) {
        return ESP_FAIL;
    }
    if (g_radio.setDio2AsRfSwitch(true) != RADIOLIB_ERR_NONE || g_radio.setCurrentLimit(140.0f) != RADIOLIB_ERR_NONE) {
        g_radio.standby();
        return ESP_FAIL;
    }
    g_ready = true;
    return ESP_OK;
}

esp_err_t Radio::sleep()
{
    return g_ready ? radio_result(g_radio.sleep()) : ESP_ERR_INVALID_STATE;
}

esp_err_t Radio::receive()
{
    return g_ready ? radio_result(g_radio.startReceive()) : ESP_ERR_INVALID_STATE;
}

esp_err_t Radio::transmit(const char* payload, std::uint32_t timeout_ms)
{
    if (!g_ready || payload == nullptr || payload[0] == '\0' || timeout_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    g_tx_done = false;
    g_radio.setPacketSentAction(on_packet_sent);
    if (g_radio.startTransmit(payload) != RADIOLIB_ERR_NONE) {
        g_radio.clearPacketSentAction();
        return ESP_FAIL;
    }

    const std::int64_t deadline = esp_timer_get_time() + static_cast<std::int64_t>(timeout_ms) * 1000;
    while (!g_tx_done && esp_timer_get_time() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    g_radio.clearPacketSentAction();
    if (!g_tx_done) {
        g_radio.standby();
        return ESP_ERR_TIMEOUT;
    }
    return radio_result(g_radio.finishTransmit());
}

esp_err_t Radio::stop()
{
    if (!g_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    g_radio.clearPacketSentAction();
    const esp_err_t result = radio_result(g_radio.standby());
    g_hal.term();
    g_ready = false;
    return result;
}

}  // namespace m5pm::lora
