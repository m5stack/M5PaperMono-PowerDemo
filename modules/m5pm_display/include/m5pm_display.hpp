#pragma once
#include <cstddef>
#include <cstdint>
#include "M5GFX.h"
#include "esp_err.h"
namespace m5pm::display {
enum class PanelState : std::uint8_t { idle, deep_sleep, power_off };
struct Controller {
    esp_err_t begin();
    esp_err_t render_white(m5gfx::epd_mode_t mode);
    esp_err_t render_png(const std::uint8_t* data, std::size_t length, m5gfx::epd_mode_t mode);
    esp_err_t render_png_counter(const std::uint8_t* data, std::size_t length, std::uint32_t counter,
                                 m5gfx::epd_mode_t mode);
    esp_err_t render_full_load_status(bool wifi_connected, std::uint32_t tx_count, bool nfc_polling,
                                      std::uint32_t nfc_detections, m5gfx::epd_mode_t mode);
    esp_err_t set_panel_state(PanelState state);
};
const char* panel_state_name(PanelState state);
}  // namespace m5pm::display
