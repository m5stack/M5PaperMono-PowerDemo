#pragma once
#include <cstdint>
#include "esp_err.h"
namespace m5pm::lora {
struct Radio {
    esp_err_t begin();
    esp_err_t sleep();
    esp_err_t receive();
    esp_err_t transmit(const char* payload, std::uint32_t timeout_ms);
    esp_err_t stop();
};
}  // namespace m5pm::lora
