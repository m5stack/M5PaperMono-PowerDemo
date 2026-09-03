#pragma once
#include "esp_err.h"
namespace m5pm::wifi {
enum class PowerSave { none, max_modem };
esp_err_t connect(PowerSave mode);
esp_err_t start_fixed_ping();
bool connected();
}  // namespace m5pm::wifi
