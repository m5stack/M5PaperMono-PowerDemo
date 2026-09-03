#include "m5pm_display.hpp"

#include "esp_log.h"
#include <cstdio>
#include <lgfx/v1/panel/Panel_SSD1677.hpp>
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>

namespace m5pm::display {
namespace {

constexpr char kTag[]                  = "m5pm_display";
constexpr std::int32_t kPortraitWidth  = 480;
constexpr std::int32_t kPortraitHeight = 800;

class PaperMonoDisplay final : public lgfx::LGFX_Device {
public:
    PaperMonoDisplay()
    {
        auto bus_config        = bus_.config();
        bus_config.spi_host    = SPI2_HOST;
        bus_config.spi_mode    = 0;
        bus_config.freq_write  = 20000000;
        bus_config.freq_read   = 1000000;
        bus_config.spi_3wire   = false;
        bus_config.use_lock    = true;
        bus_config.dma_channel = SPI_DMA_CH_AUTO;
        bus_config.pin_sclk    = GPIO_NUM_15;
        bus_config.pin_mosi    = GPIO_NUM_14;
        bus_config.pin_miso    = GPIO_NUM_NC;
        bus_config.pin_dc      = GPIO_NUM_17;
        bus_.config(bus_config);
        panel_.bus(&bus_);

        auto panel_config            = panel_.config();
        panel_config.pin_cs          = GPIO_NUM_16;
        panel_config.pin_rst         = GPIO_NUM_NC;
        panel_config.pin_busy        = GPIO_NUM_18;
        panel_config.panel_width     = 800;
        panel_config.panel_height    = 480;
        panel_config.memory_width    = 800;
        panel_config.memory_height   = 480;
        panel_config.offset_x        = 0;
        panel_config.offset_y        = 0;
        panel_config.offset_rotation = 3;
        panel_config.readable        = false;
        panel_config.invert          = false;
        panel_config.bus_shared      = false;
        panel_.config(panel_config);
        panel(&panel_);
    }

private:
    lgfx::Bus_SPI bus_;
    lgfx::Panel_SSD1677_4Gray panel_;
};

PaperMonoDisplay g_display;
bool g_ready = false;

}  // namespace

esp_err_t Controller::begin()
{
    ESP_LOGI(kTag, "init_start SPI=2 SCLK=15 MOSI=14 DC=17 CS=16 BUSY=18 3WIRE=0 WRITE_HZ=20000000 READ_HZ=1000000");
    if (!g_display.init()) {
        ESP_LOGE(kTag, "init_failed");
        return ESP_FAIL;
    }
    // The panel offset maps application rotation 0 to the product's native
    // 480x800 portrait orientation used by the M5Stack UI assets.
    g_display.setRotation(0);
    g_display.setAutoDisplay(false);
    g_ready = g_display.width() == kPortraitWidth && g_display.height() == kPortraitHeight;
    ESP_LOGI(kTag, "init_result ready=%d size=%dx%d", g_ready, g_display.width(), g_display.height());
    return g_ready ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t Controller::render_white(m5gfx::epd_mode_t mode)
{
    if (!g_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(kTag, "refresh_start mode=%d", static_cast<int>(mode));
    g_display.setEpdMode(mode);
    g_display.fillScreen(TFT_WHITE);
    g_display.display(0, 0, g_display.width(), g_display.height());
    g_display.waitDisplay();
    // waitDisplay() performs the driver's bounded BUSY wait. A second
    // immediate BUSY sample can observe the controller's post-refresh edge
    // and incorrectly report a timeout even though the frame was transferred.
    ESP_LOGI(kTag, "refresh_wait_complete");
    return ESP_OK;
}

esp_err_t Controller::render_png(const std::uint8_t* data, std::size_t length, m5gfx::epd_mode_t mode)
{
    if (!g_ready || data == nullptr || length == 0) return ESP_ERR_INVALID_ARG;
    g_display.setEpdMode(mode);
    g_display.fillScreen(TFT_WHITE);
    if (!g_display.drawPng(data, length, 0, 0, g_display.width(), g_display.height(), 0, 0, 0.0f, 0.0f,
                           datum_t::middle_center)) {
        ESP_LOGE(kTag, "png_decode_failed");
        return ESP_ERR_INVALID_RESPONSE;
    }
    g_display.display(0, 0, g_display.width(), g_display.height());
    g_display.waitDisplay();
    return ESP_OK;
}

esp_err_t Controller::render_png_counter(const std::uint8_t* data, std::size_t length, std::uint32_t counter,
                                         m5gfx::epd_mode_t mode)
{
    if (!g_ready || data == nullptr || length == 0) return ESP_ERR_INVALID_ARG;
    char counter_text[32] = {};
    std::snprintf(counter_text, sizeof(counter_text), "Refresh: %u", static_cast<unsigned>(counter));
    ESP_LOGI(kTag, "refresh_counter=%u mode=%d area=%dx%d", static_cast<unsigned>(counter), static_cast<int>(mode),
             g_display.width(), g_display.height());
    g_display.setEpdMode(mode);
    g_display.fillScreen(TFT_WHITE);
    if (!g_display.drawPng(data, length, 0, 0, g_display.width(), g_display.height(), 0, 0, 0.0f, 0.0f,
                           datum_t::middle_center)) {
        ESP_LOGE(kTag, "png_decode_failed");
        return ESP_ERR_INVALID_RESPONSE;
    }
    g_display.setTextDatum(lgfx::v1::textdatum_t::top_right);
    g_display.setTextColor(TFT_BLACK, TFT_WHITE);
    g_display.setTextSize(2);
    g_display.drawString(counter_text, g_display.width() - 12, 12);
    g_display.display(0, 0, g_display.width(), g_display.height());
    g_display.waitDisplay();
    ESP_LOGI(kTag, "refresh_wait_complete counter=%u", static_cast<unsigned>(counter));
    return ESP_OK;
}

esp_err_t Controller::render_full_load_status(bool wifi_connected, std::uint32_t tx_count, bool nfc_polling,
                                              std::uint32_t nfc_detections, m5gfx::epd_mode_t mode)
{
    if (!g_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    char line[64] = {};
    ESP_LOGI(kTag, "full_load_status wifi=%d tx=%u nfc=%d detections=%u", wifi_connected ? 1 : 0,
             static_cast<unsigned>(tx_count), nfc_polling ? 1 : 0, static_cast<unsigned>(nfc_detections));
    g_display.setEpdMode(mode);
    g_display.fillScreen(TFT_WHITE);
    g_display.setTextDatum(lgfx::v1::textdatum_t::top_left);
    g_display.setTextColor(TFT_BLACK, TFT_WHITE);
    g_display.setTextSize(2);
    g_display.drawString("Full Load Test", 20, 20);
    g_display.drawString("Backlight: 100%", 20, 100);
    g_display.drawString(wifi_connected ? "WiFi: Connected" : "WiFi: Disconnected", 20, 170);
    g_display.drawString("LoRa TX: Running (22 dBm)", 20, 240);
    std::snprintf(line, sizeof(line), "TX Count: %u", static_cast<unsigned>(tx_count));
    g_display.drawString(line, 20, 310);
    g_display.drawString(nfc_polling ? "NFC: Polling (NFC-A)" : "NFC: OFF", 20, 380);
    std::snprintf(line, sizeof(line), "NFC Detections: %u", static_cast<unsigned>(nfc_detections));
    g_display.drawString(line, 20, 450);
    g_display.setTextColor(0x8410, TFT_WHITE);
    g_display.drawString("Running", 20, 550);
    g_display.setTextSize(1);
    g_display.drawString("Button 1 / Button 2: Stop", 20, 650);
    g_display.drawString("Tap title: Stop", 20, 700);
    g_display.display(0, 0, g_display.width(), g_display.height());
    g_display.waitDisplay();
    return ESP_OK;
}

esp_err_t Controller::set_panel_state(PanelState state)
{
    if (!g_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    switch (state) {
        case PanelState::idle:
            g_display.powerSave(true);
            break;
        case PanelState::deep_sleep:
        case PanelState::power_off:
            g_display.sleep();
            break;
    }
    return ESP_OK;
}

const char* panel_state_name(PanelState state)
{
    switch (state) {
        case PanelState::idle:
            return "idle";
        case PanelState::deep_sleep:
            return "deep_sleep";
        case PanelState::power_off:
            return "power_off";
    }
    return "unknown";
}

}  // namespace m5pm::display
