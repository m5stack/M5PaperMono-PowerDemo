#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"
#include "m5pm_power.hpp"
#include "m5pm_test_support.hpp"

namespace {
constexpr char kTag[]              = "l1_standby";
constexpr TickType_t kPrepareDelay = pdMS_TO_TICKS(3000);
}  // namespace

extern "C" void app_main(void)
{
    m5pm::test_support::TestContext context = {};
    ESP_ERROR_CHECK(m5pm::test_support::initialize(&context));

    const esp_err_t i2c_result = m5pm::board::initialize_i2c();
    if (i2c_result != ESP_OK) {
        ESP_LOGE(kTag, "BOARD_FAIL stage=i2c error=%s", esp_err_to_name(i2c_result));
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "i2c_initialization_failed");
        m5pm::test_support::hold_safe_idle();
    }

    i2c_bus_handle_t bus         = nullptr;
    const esp_err_t claim_result = m5pm::board::acquire_i2c(&bus);
    if (claim_result != ESP_OK) {
        ESP_LOGE(kTag, "BOARD_FAIL stage=pm1_bus error=%s", esp_err_to_name(claim_result));
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "pm1_bus_claim_failed");
        m5pm::test_support::hold_safe_idle();
    }

    m5pm::power::PowerController power;
    const m5pm::power::PowerResult begin_result = power.begin(bus);
    if (!begin_result.ok()) {
        ESP_LOGE(kTag, "BOARD_FAIL stage=pm1_begin error=%s raw=%d", esp_err_to_name(begin_result.error),
                 begin_result.raw_error);
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "pm1_initialization_failed");
        m5pm::test_support::hold_safe_idle();
    }
    ESP_LOGI(kTag, "BOARD_OK PM1=1");

    esp_log_level_set("M5PM1_I2C", ESP_LOG_ERROR);
    esp_log_level_set("M5PM1_PWR", ESP_LOG_ERROR);
    ESP_ERROR_CHECK(m5pm::test_support::mark_test_ready());
    vTaskDelay(kPrepareDelay);

    const m5pm::power::PowerResult standby_result = power.enter_l1();
    if (!standby_result.ok()) {
        ESP_LOGE(kTag, "TEST_FAIL stage=l1_enter error=%s raw=%d", esp_err_to_name(standby_result.error),
                 standby_result.raw_error);
        m5pm::test_support::report_final(m5pm::test_support::FinalStatus::fail, "l1_standby_failed");
        m5pm::test_support::hold_safe_idle();
    }

    // A successful shutdown removes the console power path while the LDO 3.3 V
    // domain remains held. No logging or bus access is allowed afterward.
    m5pm::test_support::hold_safe_idle();
}
