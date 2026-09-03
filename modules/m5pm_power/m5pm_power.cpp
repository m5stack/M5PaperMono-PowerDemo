#include "m5pm_power.hpp"

#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m5pm_board.hpp"

namespace m5pm::power {
namespace {

#if M5PM_POWER_HAS_IOE1
constexpr std::uint8_t kTouchResetPin  = M5IOE1_PIN_6;
constexpr std::uint8_t kTouchEnablePin = M5IOE1_PIN_13;
#endif
constexpr TickType_t kTouchResetLowDelay    = pdMS_TO_TICKS(20);
constexpr TickType_t kTouchResetReadyDelay  = pdMS_TO_TICKS(300);
constexpr TickType_t kIoe1ResetReadyTimeout = pdMS_TO_TICKS(1200);
constexpr TickType_t kIoe1ResetPollInterval = pdMS_TO_TICKS(20);

#if M5PM_POWER_HAS_IOE1
esp_err_t wait_for_ioe1_ready(i2c_bus_handle_t bus)
{
    i2c_bus_device_handle_t device = i2c_bus_device_create(bus, m5pm::board::kIoe1Address,
                                                           static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard));
    if (device == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    const TickType_t start = xTaskGetTickCount();
    bool ready             = false;
    do {
        std::uint8_t uid[2] = {};
        ready               = i2c_bus_read_bytes(device, M5IOE1_REG_UID_L, sizeof(uid), uid) == ESP_OK;
        if (!ready) {
            vTaskDelay(kIoe1ResetPollInterval);
        }
    } while (!ready && (xTaskGetTickCount() - start) < kIoe1ResetReadyTimeout);

    const esp_err_t cleanup_error = i2c_bus_device_delete(&device);
    if (cleanup_error != ESP_OK) {
        return cleanup_error;
    }
    return ready ? ESP_OK : ESP_ERR_TIMEOUT;
}
#endif

esp_err_t map_error(m5pm1_err_t error)
{
    switch (error) {
        case M5PM1_OK:
            return ESP_OK;
        case M5PM1_ERR_INVALID_ARG:
            return ESP_ERR_INVALID_ARG;
        case M5PM1_ERR_NOT_INIT:
            return ESP_ERR_INVALID_STATE;
        case M5PM1_ERR_NOT_SUPPORTED:
            return ESP_ERR_NOT_SUPPORTED;
        case M5PM1_ERR_TIMEOUT:
            return ESP_ERR_TIMEOUT;
        default:
            return ESP_FAIL;
    }
}

#if M5PM_POWER_HAS_IOE1
esp_err_t map_error(m5ioe1_err_t error)
{
    switch (error) {
        case M5IOE1_OK:
            return ESP_OK;
        case M5IOE1_ERR_INVALID_ARG:
            return ESP_ERR_INVALID_ARG;
        case M5IOE1_ERR_NOT_INIT:
            return ESP_ERR_INVALID_STATE;
        case M5IOE1_ERR_NOT_SUPPORTED:
            return ESP_ERR_NOT_SUPPORTED;
        case M5IOE1_ERR_TIMEOUT:
            return ESP_ERR_TIMEOUT;
        default:
            return ESP_FAIL;
    }
}
#endif

esp_err_t delete_devices(i2c_bus_device_handle_t* imu, i2c_bus_device_handle_t* nfc,
                         i2c_bus_device_handle_t* touch = nullptr)
{
    esp_err_t first_error = ESP_OK;
    if (touch != nullptr && *touch != nullptr) {
        first_error = i2c_bus_device_delete(touch);
    }
    if (nfc != nullptr && *nfc != nullptr) {
        const esp_err_t error = i2c_bus_device_delete(nfc);
        if (first_error == ESP_OK) {
            first_error = error;
        }
    }
    if (imu != nullptr && *imu != nullptr) {
        const esp_err_t error = i2c_bus_device_delete(imu);
        if (first_error == ESP_OK) {
            first_error = error;
        }
    }
    return first_error;
}

}  // namespace

PowerResult PowerController::result(m5pm1_err_t raw) const
{
    return {map_error(raw), static_cast<int>(raw)};
}

#if M5PM_POWER_HAS_IOE1
PowerResult PowerController::result(m5ioe1_err_t raw) const
{
    return {map_error(raw), static_cast<int>(raw)};
}
#endif

PowerResult PowerController::begin(i2c_bus_handle_t bus, std::uint32_t speed_hz)
{
    if (bus == nullptr || (speed_hz != 100000U && speed_hz != 400000U)) {
        return {ESP_ERR_INVALID_ARG, M5PM1_ERR_INVALID_ARG};
    }

    bus_                           = nullptr;
    ioe1_initialized_              = false;
    imu_shutdown_prepared_         = false;
    l2_prepared_                   = false;
    l2_mode_                       = L2Mode::no_wake;
    const PowerResult begin_result = result(pm1_.begin(bus, m5pm::board::kPm1Address, speed_hz));
    initialized_                   = begin_result.ok();
    if (initialized_) {
        bus_ = bus;
    }
    return begin_result;
}

PowerResult PowerController::begin_l2(i2c_bus_handle_t bus, std::uint32_t speed_hz)
{
    PowerResult step = begin(bus, speed_hz);
    if (!step.ok()) {
        return step;
    }

    // Wake PM1's I2C interface explicitly before applying the final L2
    // idle-sleep setting. The first write can be consumed while recovering
    // from a previous sleep cycle, so the reference sequence writes twice.
    step = result(pm1_.setI2cSleepTime(0));
    if (!step.ok()) {
        step.step = "pm1_i2c_wake_first";
        return step;
    }
    step = result(pm1_.setI2cSleepTime(0));
    if (!step.ok()) {
        step.step = "pm1_i2c_wake_second";
        return step;
    }

    step = result(pm1_.wdtSet(0));
    if (!step.ok()) {
        return step;
    }
    std::uint8_t watchdog_count = 0;
    step                        = result(pm1_.wdtGetCount(&watchdog_count));
    if (!step.ok()) {
        return step;
    }
    if (watchdog_count != 0U) {
        return {ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED, "pm1_watchdog_verify"};
    }

    step = result(pm1_.timerClear());
    if (!step.ok()) {
        step.step = "pm1_timer_disable";
        return step;
    }

    step = result(pm1_.ldoSetPowerHold(false));
    if (!step.ok()) {
        return step;
    }
    bool ldo_hold = true;
    step          = result(pm1_.ldoGetPowerHold(&ldo_hold));
    if (!step.ok()) {
        return step;
    }
    if (ldo_hold) {
        return {ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED, "pm1_ldo_hold_verify"};
    }

#if M5PM_POWER_HAS_IOE1
    step              = result(ioe1_.begin(bus_, m5pm::board::kIoe1Address, speed_hz, M5IOE1_INT_MODE_DISABLED));
    ioe1_initialized_ = step.ok();
    return step;
#else
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#endif
}

PowerResult PowerController::enter_l0()
{
    if (!initialized_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT};
    }

    PowerResult step = result(pm1_.setChargeEnable(false));
    if (!step.ok()) {
        return step;
    }

    step = result(pm1_.ldoSetPowerHold(false));
    if (!step.ok()) {
        return step;
    }

    // shutdown() must be the final PM1 transaction in this path.
    return result(pm1_.shutdown());
}

PowerResult PowerController::enter_l1()
{
    if (!initialized_ || bus_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT};
    }

    constexpr std::uint32_t kPeripheralSpeed = static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard);
    i2c_bus_device_handle_t imu              = i2c_bus_device_create(bus_, m5pm::board::kImuAddress, kPeripheralSpeed);
    if (imu == nullptr) {
        return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM};
    }
    PowerResult step = result(pm1_.setChargeEnable(false));
    if (!step.ok()) {
        delete_devices(&imu, nullptr);
        return step;
    }

    step = result(pm1_.ldoSetPowerHold(true));
    if (!step.ok()) {
        delete_devices(&imu, nullptr);
        return step;
    }

    esp_err_t peripheral_error = i2c_bus_write_byte(imu, 0x7C, 0x01);
    if (peripheral_error == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));
        peripheral_error = i2c_bus_write_byte(imu, 0x7D, 0x00);
    }
    if (peripheral_error != ESP_OK) {
        delete_devices(&imu, nullptr);
        return {peripheral_error, static_cast<int>(peripheral_error)};
    }

    // NFC is powered from 3V3_L2. PM1 L1 retains the 3V3_L1 domain, so NFC is
    // not accessed or initialized in this path.
    const esp_err_t cleanup_error = delete_devices(&imu, nullptr);
    if (cleanup_error != ESP_OK) {
        return {cleanup_error, static_cast<int>(cleanup_error)};
    }

    // shutdown() must be the final bus transaction in this path.
    return result(pm1_.shutdown());
}

PowerResult PowerController::capture_wake_status(WakeStatus* status)
{
    if (!initialized_ || status == nullptr) {
        return {ESP_ERR_INVALID_ARG, M5PM1_ERR_INVALID_ARG, "wake_status_argument"};
    }

    PowerResult step = result(pm1_.getWakeSource(&status->pm1_wake, M5PM1_CLEAN_NONE));
    if (!step.ok()) {
        step.step = "pm1_wake_status";
        return step;
    }
    step = result(pm1_.irqGetGpioStatus(&status->pm1_gpio_irq, M5PM1_CLEAN_NONE));
    if (!step.ok()) {
        step.step = "pm1_gpio_irq_status";
        return step;
    }
    step = result(pm1_.irqGetSysStatus(&status->pm1_system_irq, M5PM1_CLEAN_NONE));
    if (!step.ok()) {
        step.step = "pm1_system_irq_status";
        return step;
    }
    step = result(pm1_.irqGetBtnStatus(&status->pm1_button_irq, M5PM1_CLEAN_NONE));
    if (!step.ok()) {
        step.step = "pm1_button_irq_status";
    }
    return step;
}

PowerResult PowerController::prepare_imu_wake_shutdown()
{
    return prepare_shutdown_wake(M5PM1_GPIO_NUM_4);
}

PowerResult PowerController::prepare_shutdown_wake(m5pm1_gpio_num_t wake_pin)
{
    if (!initialized_ || bus_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "state_check"};
    }
    if (imu_shutdown_prepared_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_INVALID_ARG, "shutdown_wake_already_prepared"};
    }

    constexpr std::uint32_t kPeripheralSpeed = static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard);
    i2c_bus_device_handle_t nfc              = i2c_bus_device_create(bus_, m5pm::board::kNfcAddress, kPeripheralSpeed);
    if (nfc == nullptr) {
        return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM, "nfc_device_create"};
    }

    const auto cleanup_failure = [&](PowerResult failure, const char* step_name) {
        i2c_bus_device_delete(&nfc);
        failure.step = step_name;
        return failure;
    };

    PowerResult step = result(pm1_.wdtSet(0));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_watchdog_disable");
    }
    std::uint8_t watchdog_count = 0;
    step                        = result(pm1_.wdtGetCount(&watchdog_count));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_watchdog_readback");
    }
    if (watchdog_count != 0U) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_watchdog_verify");
    }

    step = result(pm1_.timerClear());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_timer_disable");
    }

    constexpr std::uint8_t kDisabledPowerMask = M5PM1_PWR_CFG_CHG_EN | M5PM1_PWR_CFG_BOOST_EN | M5PM1_PWR_CFG_LED_CTRL;
    step                                      = result(pm1_.setPowerConfig(kDisabledPowerMask, 0));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_power_disable");
    }
    std::uint8_t power_config = 0;
    step                      = result(pm1_.getPowerConfig(&power_config));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_power_readback");
    }
    if ((power_config & kDisabledPowerMask) != 0U) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_power_verify");
    }

    // RTC wake does not need the PM1 LDO retained, while IMU wake must keep the
    // sensor rail alive until BMI270 asserts GPIO4 after shutdown.
    const bool retain_ldo = wake_pin == M5PM1_GPIO_NUM_4;
    step                  = result(pm1_.ldoSetPowerHold(retain_ldo));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_ldo_hold_configure");
    }
    bool ldo_hold = true;
    step          = result(pm1_.ldoGetPowerHold(&ldo_hold));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_ldo_hold_readback");
    }
    if (ldo_hold != retain_ldo) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_ldo_hold_verify");
    }

    step = result(pm1_.setPwmDuty(M5PM1_PWM_CH_0, 0, false, false));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_pwm0_disable");
    }
    step = result(pm1_.setPwmDuty(M5PM1_PWM_CH_1, 0, false, false));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_pwm1_disable");
    }

    step = result(pm1_.irqSetGpioMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_mask");
    }
    step = result(pm1_.irqSetSysMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_mask");
    }
    step = result(pm1_.irqSetBtnMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_mask");
    }

    constexpr struct {
        m5pm1_gpio_num_t pin;
        const char* step;
    } kPm1Pins[] = {
        {M5PM1_GPIO_NUM_0, "pm1_gpio0_safe"}, {M5PM1_GPIO_NUM_1, "pm1_gpio1_safe"},
        {M5PM1_GPIO_NUM_2, "pm1_gpio2_safe"}, {M5PM1_GPIO_NUM_3, "pm1_gpio3_safe"},
        {M5PM1_GPIO_NUM_4, "pm1_gpio4_safe"},
    };
    for (const auto& pin_config : kPm1Pins) {
        step = result(pm1_.gpioSetWakeEnable(pin_config.pin, false));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
        step = result(pm1_.gpioSetFunc(pin_config.pin, M5PM1_GPIO_FUNC_GPIO));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
        step = result(
            pm1_.gpioSet(pin_config.pin, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_OPENDRAIN));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
    }

    const m5pm1_gpio_drive_t wake_drive =
        wake_pin == M5PM1_GPIO_NUM_4 ? M5PM1_GPIO_DRIVE_PUSHPULL : M5PM1_GPIO_DRIVE_OPENDRAIN;
    step = result(pm1_.gpioSet(wake_pin, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_UP, wake_drive));
    if (!step.ok()) {
        return cleanup_failure(step, wake_pin == M5PM1_GPIO_NUM_4 ? "pm1_gpio4_wake_config" : "pm1_gpio0_wake_config");
    }
    step = result(pm1_.gpioSetWakeEdge(wake_pin, M5PM1_GPIO_WAKE_FALLING));
    if (!step.ok()) {
        return cleanup_failure(step, wake_pin == M5PM1_GPIO_NUM_4 ? "pm1_gpio4_wake_edge" : "pm1_gpio0_wake_edge");
    }
    step = result(pm1_.gpioSetWakeEnable(wake_pin, true));
    if (!step.ok()) {
        return cleanup_failure(step, wake_pin == M5PM1_GPIO_NUM_4 ? "pm1_gpio4_wake_enable" : "pm1_gpio0_wake_enable");
    }
    step = result(
        pm1_.irqSetGpioMask(wake_pin == M5PM1_GPIO_NUM_4 ? M5PM1_IRQ_GPIO4 : M5PM1_IRQ_GPIO0, M5PM1_IRQ_MASK_DISABLE));
    if (!step.ok()) {
        return cleanup_failure(step, wake_pin == M5PM1_GPIO_NUM_4 ? "pm1_gpio4_irq_unmask" : "pm1_gpio0_irq_unmask");
    }

    std::uint8_t discarded_wake = 0;
    step                        = result(pm1_.getWakeSource(&discarded_wake, M5PM1_CLEAN_ALL));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_wake_clear");
    }
    step = result(pm1_.irqClearGpioAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_clear");
    }
    step = result(pm1_.irqClearSysAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_clear");
    }
    step = result(pm1_.irqClearBtnAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_clear");
    }

    std::uint8_t gpio_irq_mask = 0;
    std::uint8_t sys_irq_mask  = 0;
    std::uint8_t btn_irq_mask  = 0;
    step                       = result(pm1_.irqGetGpioMaskBits(&gpio_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_mask_readback");
    }
    step = result(pm1_.irqGetSysMaskBits(&sys_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_mask_readback");
    }
    step = result(pm1_.irqGetBtnMaskBits(&btn_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_mask_readback");
    }
    const std::uint8_t kExpectedGpioMask = static_cast<std::uint8_t>(
        M5PM1_IRQ_GPIO_ALL & ~(wake_pin == M5PM1_GPIO_NUM_4 ? M5PM1_IRQ_GPIO4 : M5PM1_IRQ_GPIO0));
    if ((gpio_irq_mask & M5PM1_IRQ_GPIO_ALL) != kExpectedGpioMask ||
        (sys_irq_mask & M5PM1_IRQ_SYS_ALL) != M5PM1_IRQ_SYS_ALL ||
        (btn_irq_mask & M5PM1_IRQ_BTN_ALL) != M5PM1_IRQ_BTN_ALL) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_irq_mask_verify");
    }

    esp_err_t peripheral_error = i2c_bus_write_byte(nfc, 0xC2, 0xC2);
    if (peripheral_error == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(10));
        peripheral_error = i2c_bus_write_byte(nfc, 0x02, 0x00);
    }
    if (peripheral_error != ESP_OK) {
        return cleanup_failure({peripheral_error, static_cast<int>(peripheral_error)}, "nfc_power_down");
    }
    const esp_err_t cleanup_error = i2c_bus_device_delete(&nfc);
    if (cleanup_error != ESP_OK) {
        return {cleanup_error, static_cast<int>(cleanup_error), "nfc_device_delete"};
    }

    imu_shutdown_prepared_ = true;
    return {};
}

PowerResult PowerController::enter_imu_wake_shutdown()
{
    if (!initialized_ || !imu_shutdown_prepared_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "shutdown_wake_state"};
    }

    PowerResult step = result(pm1_.shutdown());
    if (!step.ok()) {
        step.step = "pm1_shutdown";
    }
    return step;
}

PowerResult PowerController::prepare_rtc_wake_shutdown()
{
    const PowerResult result = prepare_shutdown_wake(M5PM1_GPIO_NUM_0);
    if (result.ok()) {
        rtc_shutdown_prepared_ = true;
        imu_shutdown_prepared_ = true;
    }
    return result;
}

PowerResult PowerController::enter_rtc_wake_shutdown()
{
    if (!initialized_ || !rtc_shutdown_prepared_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "rtc_shutdown_wake_state"};
    }
    PowerResult result = PowerController::enter_imu_wake_shutdown();
    if (!result.ok()) {
        result.step = "pm1_shutdown";
    }
    return result;
}

PowerResult PowerController::prepare_l2(L2Mode mode)
{
#if !M5PM_POWER_HAS_IOE1
    (void)mode;
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    if (!initialized_ || !ioe1_initialized_ || bus_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "state_check"};
    }
    if (l2_prepared_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_INVALID_ARG, "l2_already_prepared"};
    }

    constexpr std::uint32_t kPeripheralSpeed = static_cast<std::uint32_t>(m5pm::board::I2cSpeed::standard);

    const auto failure_at = [](PowerResult failure, const char* step) {
        failure.step = step;
        return failure;
    };

    i2c_bus_device_handle_t imu = nullptr;
    if (mode != L2Mode::imu_wake) {
        imu = i2c_bus_device_create(bus_, m5pm::board::kImuAddress, kPeripheralSpeed);
        if (imu == nullptr) {
            return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM, "imu_device_create"};
        }
    }
    i2c_bus_device_handle_t nfc = i2c_bus_device_create(bus_, m5pm::board::kNfcAddress, kPeripheralSpeed);
    if (nfc == nullptr) {
        const esp_err_t cleanup_error = delete_devices(&imu, &nfc);
        return {cleanup_error == ESP_OK ? ESP_ERR_NO_MEM : cleanup_error,
                cleanup_error == ESP_OK ? ESP_ERR_NO_MEM : static_cast<int>(cleanup_error),
                cleanup_error == ESP_OK ? "nfc_device_create" : "imu_device_delete"};
    }
    i2c_bus_device_handle_t touch = i2c_bus_device_create(bus_, m5pm::board::kTouchAddress, kPeripheralSpeed);
    if (touch == nullptr) {
        const esp_err_t cleanup_error = delete_devices(&imu, &nfc);
        return {cleanup_error == ESP_OK ? ESP_ERR_NO_MEM : cleanup_error,
                cleanup_error == ESP_OK ? ESP_ERR_NO_MEM : static_cast<int>(cleanup_error),
                cleanup_error == ESP_OK ? "touch_device_create" : "temporary_device_delete"};
    }

    const auto cleanup_failure = [&](PowerResult failure, const char* step_name) {
        delete_devices(&imu, &nfc, &touch);
        return failure_at(failure, step_name);
    };

    PowerResult step = result(pm1_.switchI2cSpeed(M5PM1_I2C_SPEED_100K));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_i2c_speed");
    }

    // Stabilize the PM1 rails before resetting IOE1. A previous shutdown test
    // may leave the retained power configuration with DCDC/LDO disabled; the
    // IOE1 reset then causes a brownout before the final L2 rail settings can
    // be applied.
    step = result(ioe1_.setI2cConfig(0, M5IOE1_I2C_SPEED_100K, M5IOE1_WAKE_EDGE_RISING, M5IOE1_PULL_DISABLED));
    if (!step.ok()) {
        return cleanup_failure(step, "ioe1_i2c_awake");
    }

    m5ioe1_err_t ioe1_error    = M5IOE1_OK;
    esp_err_t peripheral_error = ESP_OK;
    if (mode != L2Mode::no_wake) {
        ioe1_.pinModeWithRes(kTouchEnablePin, OUTPUT, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_mode");
        }
        step = result(ioe1_.setDriveMode(kTouchEnablePin, M5IOE1_DRIVE_PUSHPULL));
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_drive");
        }
        ioe1_.digitalWriteWithRes(kTouchEnablePin, HIGH, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_high");
        }

        ioe1_.pinModeWithRes(kTouchResetPin, OUTPUT, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_mode");
        }
        step = result(ioe1_.setDriveMode(kTouchResetPin, M5IOE1_DRIVE_PUSHPULL));
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_drive");
        }
        ioe1_.digitalWriteWithRes(kTouchResetPin, LOW, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_low");
        }
        vTaskDelay(kTouchResetLowDelay);
        ioe1_.digitalWriteWithRes(kTouchResetPin, HIGH, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_high");
        }
        vTaskDelay(kTouchResetReadyDelay);

        peripheral_error = i2c_bus_write_byte(touch, 0xA5, 0x03);
        if (peripheral_error != ESP_OK) {
            return cleanup_failure({peripheral_error, static_cast<int>(peripheral_error)}, "touch_hibernate");
        }
    }

    // For the no-wake L2 path, match the reference ordering: reset IOE1 first,
    // then hibernate Touch after the expander has returned and been reopened.
    if (mode == L2Mode::no_wake) {
        const esp_err_t touch_delete_error = i2c_bus_device_delete(&touch);
        if (touch_delete_error != ESP_OK) {
            return cleanup_failure({touch_delete_error, static_cast<int>(touch_delete_error)}, "touch_device_delete");
        }
    }
    step = result(ioe1_.factoryReset());
    if (!step.ok()) {
        return cleanup_failure(step, "ioe1_factory_reset");
    }
    ioe1_initialized_ = false;

    const esp_err_t ready_error = wait_for_ioe1_ready(bus_);
    if (ready_error != ESP_OK) {
        return cleanup_failure({ready_error, static_cast<int>(ready_error)}, "ioe1_reset_ready");
    }

    step = result(ioe1_.begin(bus_, m5pm::board::kIoe1Address, kPeripheralSpeed, M5IOE1_INT_MODE_DISABLED));
    if (!step.ok()) {
        return cleanup_failure(step, "ioe1_reinitialize");
    }
    ioe1_initialized_ = true;

    if (mode == L2Mode::no_wake) {
        // The reference Touch driver initializes the controller after IOE1 is
        // reopened. On this board the equivalent hardware step is to reassert
        // the Touch enable/reset pins before issuing the hibernate command.
        ioe1_.pinModeWithRes(kTouchEnablePin, OUTPUT, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_mode");
        }
        step = result(ioe1_.setDriveMode(kTouchEnablePin, M5IOE1_DRIVE_PUSHPULL));
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_drive");
        }
        ioe1_.digitalWriteWithRes(kTouchEnablePin, HIGH, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_enable_high");
        }
        ioe1_.pinModeWithRes(kTouchResetPin, OUTPUT, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_mode");
        }
        step = result(ioe1_.setDriveMode(kTouchResetPin, M5IOE1_DRIVE_PUSHPULL));
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_drive");
        }
        ioe1_.digitalWriteWithRes(kTouchResetPin, LOW, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_low");
        }
        vTaskDelay(kTouchResetLowDelay);
        ioe1_.digitalWriteWithRes(kTouchResetPin, HIGH, &ioe1_error);
        step = result(ioe1_error);
        if (!step.ok()) {
            return cleanup_failure(step, "touch_reset_high");
        }
        vTaskDelay(kTouchResetReadyDelay);

        touch = i2c_bus_device_create(bus_, m5pm::board::kTouchAddress, kPeripheralSpeed);
        if (touch == nullptr) {
            return cleanup_failure({ESP_ERR_NO_MEM, ESP_ERR_NO_MEM}, "touch_device_recreate");
        }
        peripheral_error = i2c_bus_write_byte(touch, 0xA5, 0x03);
        if (peripheral_error != ESP_OK) {
            return cleanup_failure({peripheral_error, static_cast<int>(peripheral_error)}, "touch_hibernate");
        }
    }

    // Keep IOE1 factory-reset GPIOs at their default high-impedance state.
    // Driving the board power-control pins here can re-enable external loads
    // and cause a supply transient before the final L2 rail configuration.

    ioe1_.pinModeWithRes(kTouchResetPin, INPUT, &ioe1_error);
    step = result(ioe1_error);
    if (!step.ok()) {
        return cleanup_failure(step, "touch_reset_release");
    }
    ioe1_.pinModeWithRes(kTouchEnablePin, INPUT, &ioe1_error);
    step = result(ioe1_error);
    if (!step.ok()) {
        return cleanup_failure(step, "touch_enable_release");
    }

    constexpr std::uint8_t kDisabledPowerMask = M5PM1_PWR_CFG_CHG_EN | M5PM1_PWR_CFG_BOOST_EN | M5PM1_PWR_CFG_LED_CTRL;
    step                                      = result(pm1_.setPowerConfig(kDisabledPowerMask, 0));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_power_disable");
    }
    std::uint8_t power_config = 0;
    step                      = result(pm1_.getPowerConfig(&power_config));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_power_readback");
    }
    if ((power_config & kDisabledPowerMask) != 0U) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_power_verify");
    }

    // Keep the PM1 LDO latched only for wake paths that require the retained
    // 3.3 V rail across Deep Sleep. RTC alarm and button wake do not depend on
    // retained PM1 power and leave the hold disabled to minimize sleep current.
    const bool retain_ldo = mode == L2Mode::imu_wake || mode == L2Mode::rtc_wake;
    step                  = result(pm1_.ldoSetPowerHold(retain_ldo));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_ldo_hold_enable");
    }
    bool ldo_hold = false;
    step          = result(pm1_.ldoGetPowerHold(&ldo_hold));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_ldo_hold_readback");
    }
    if (ldo_hold != retain_ldo) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_ldo_hold_verify");
    }

    // PWM enable bits survive an ESP32 reset and prevent PM1 I2C idle sleep.

    step = result(pm1_.irqSetGpioMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_mask");
    }
    step = result(pm1_.irqSetSysMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_mask");
    }
    step = result(pm1_.irqSetBtnMaskAll(M5PM1_IRQ_MASK_ENABLE));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_mask");
    }

    constexpr struct {
        m5pm1_gpio_num_t pin;
        const char* step;
    } kPm1Pins[] = {
        {M5PM1_GPIO_NUM_0, "pm1_gpio0_safe"}, {M5PM1_GPIO_NUM_1, "pm1_gpio1_safe"},
        {M5PM1_GPIO_NUM_2, "pm1_gpio2_safe"}, {M5PM1_GPIO_NUM_3, "pm1_gpio3_safe"},
        {M5PM1_GPIO_NUM_4, "pm1_gpio4_safe"},
    };
    for (const auto& pin_config : kPm1Pins) {
        step = result(pm1_.gpioSetWakeEnable(pin_config.pin, false));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
        step = result(pm1_.gpioSetFunc(pin_config.pin, M5PM1_GPIO_FUNC_GPIO));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
        step = result(
            pm1_.gpioSet(pin_config.pin, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_OPENDRAIN));
        if (!step.ok()) {
            return cleanup_failure(step, pin_config.step);
        }
    }

    if (mode == L2Mode::imu_wake || mode == L2Mode::rtc_alarm_wake || mode == L2Mode::rtc_wake) {
        step = result(
            pm1_.gpioSet(M5PM1_GPIO_NUM_1, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_UP, M5PM1_GPIO_DRIVE_PUSHPULL));
        if (!step.ok()) {
            return cleanup_failure(step, "pm1_gpio1_irq_config");
        }
        step = result(pm1_.gpioSetFunc(M5PM1_GPIO_NUM_1, M5PM1_GPIO_FUNC_IRQ));
        if (!step.ok()) {
            return cleanup_failure(step, "pm1_gpio1_irq_function");
        }
    }

    if (mode == L2Mode::imu_wake || mode == L2Mode::rtc_alarm_wake || mode == L2Mode::rtc_wake) {
        const m5pm1_gpio_num_t wake_pin = mode == L2Mode::imu_wake ? M5PM1_GPIO_NUM_4 : M5PM1_GPIO_NUM_0;
        const m5pm1_irq_gpio_t wake_irq = mode == L2Mode::imu_wake ? M5PM1_IRQ_GPIO4 : M5PM1_IRQ_GPIO0;
        step = result(pm1_.gpioSet(wake_pin, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_UP, M5PM1_GPIO_DRIVE_OPENDRAIN));
        if (!step.ok()) {
            return cleanup_failure(step, mode == L2Mode::imu_wake ? "pm1_gpio4_wake_config" : "pm1_gpio0_wake_config");
        }
        step = result(pm1_.gpioSetWakeEdge(wake_pin, M5PM1_GPIO_WAKE_FALLING));
        if (!step.ok()) {
            return cleanup_failure(step, mode == L2Mode::imu_wake ? "pm1_gpio4_wake_edge" : "pm1_gpio0_wake_edge");
        }
        step = result(pm1_.gpioSetWakeEnable(wake_pin, true));
        if (!step.ok()) {
            return cleanup_failure(step, mode == L2Mode::imu_wake ? "pm1_gpio4_wake_enable" : "pm1_gpio0_wake_enable");
        }
        step = result(pm1_.irqSetGpioMask(wake_irq, M5PM1_IRQ_MASK_DISABLE));
        if (!step.ok()) {
            return cleanup_failure(step, mode == L2Mode::imu_wake ? "pm1_gpio4_irq_unmask" : "pm1_gpio0_irq_unmask");
        }
    }

    std::uint8_t discarded_wake = 0;
    step                        = result(pm1_.getWakeSource(&discarded_wake, M5PM1_CLEAN_ALL));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_wake_clear");
    }
    step = result(pm1_.irqClearGpioAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_clear");
    }
    step = result(pm1_.irqClearSysAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_clear");
    }
    step = result(pm1_.irqClearBtnAll());
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_clear");
    }

    std::uint8_t gpio_irq_mask = 0;
    std::uint8_t sys_irq_mask  = 0;
    std::uint8_t btn_irq_mask  = 0;
    step                       = result(pm1_.irqGetGpioMaskBits(&gpio_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_gpio_irq_mask_readback");
    }
    step = result(pm1_.irqGetSysMaskBits(&sys_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_sys_irq_mask_readback");
    }
    step = result(pm1_.irqGetBtnMaskBits(&btn_irq_mask));
    if (!step.ok()) {
        return cleanup_failure(step, "pm1_btn_irq_mask_readback");
    }
    const std::uint8_t expected_gpio_mask = mode == L2Mode::imu_wake
                                                ? static_cast<std::uint8_t>(M5PM1_IRQ_GPIO_ALL & ~M5PM1_IRQ_GPIO4)
                                            : (mode == L2Mode::rtc_alarm_wake || mode == L2Mode::rtc_wake)
                                                ? static_cast<std::uint8_t>(M5PM1_IRQ_GPIO_ALL & ~M5PM1_IRQ_GPIO0)
                                                : static_cast<std::uint8_t>(M5PM1_IRQ_GPIO_ALL);
    if ((gpio_irq_mask & M5PM1_IRQ_GPIO_ALL) != expected_gpio_mask ||
        (sys_irq_mask & M5PM1_IRQ_SYS_ALL) != M5PM1_IRQ_SYS_ALL ||
        (btn_irq_mask & M5PM1_IRQ_BTN_ALL) != M5PM1_IRQ_BTN_ALL) {
        return cleanup_failure({ESP_ERR_INVALID_RESPONSE, M5PM1_ERR_VERIFY_FAILED}, "pm1_irq_mask_verify");
    }

    const char* peripheral_step = "none";
    if (mode != L2Mode::imu_wake) {
        peripheral_step  = "imu_power_conf";
        peripheral_error = i2c_bus_write_byte(imu, 0x7C, 0x01);
        if (peripheral_error == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(100));
            peripheral_step  = "imu_power_ctrl";
            peripheral_error = i2c_bus_write_byte(imu, 0x7D, 0x00);
        }
        if (peripheral_error == ESP_OK) {
            peripheral_step         = "imu_power_conf_readback";
            std::uint8_t power_conf = 0;
            std::uint8_t power_ctrl = 0;
            peripheral_error        = i2c_bus_read_byte(imu, 0x7C, &power_conf);
            if (peripheral_error == ESP_OK) {
                peripheral_step  = "imu_power_ctrl_readback";
                peripheral_error = i2c_bus_read_byte(imu, 0x7D, &power_ctrl);
            }
            if (peripheral_error == ESP_OK && (power_conf != 0x01U || power_ctrl != 0x00U)) {
                return cleanup_failure({ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE}, "imu_power_verify");
            }
        }
    }
    if (peripheral_error == ESP_OK) {
        peripheral_step  = "nfc_unlock";
        peripheral_error = i2c_bus_write_byte(nfc, 0xC2, 0xC2);
    }
    if (peripheral_error == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(10));
        peripheral_step  = "nfc_power_down";
        peripheral_error = i2c_bus_write_byte(nfc, 0x02, 0x00);
    }
    if (peripheral_error != ESP_OK) {
        return cleanup_failure({peripheral_error, static_cast<int>(peripheral_error)}, peripheral_step);
    }

    esp_err_t cleanup_error  = ESP_OK;
    const char* cleanup_step = "none";
    const auto delete_device = [&](i2c_bus_device_handle_t* device, const char* step_name) {
        if (*device == nullptr) {
            return;
        }
        const esp_err_t delete_error = i2c_bus_device_delete(device);
        if (cleanup_error == ESP_OK && delete_error != ESP_OK) {
            cleanup_error = delete_error;
            cleanup_step  = step_name;
        }
    };
    delete_device(&touch, "touch_device_delete");
    delete_device(&nfc, "nfc_device_delete");
    delete_device(&imu, "imu_device_delete");
    if (cleanup_error != ESP_OK) {
        return {cleanup_error, static_cast<int>(cleanup_error), cleanup_step};
    }

    l2_prepared_ = true;
    l2_mode_     = mode;
    return {};
#endif
}

PowerResult PowerController::finish_l2(L2Mode mode)
{
    if (!l2_prepared_ || l2_mode_ != mode) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "l2_finish_state"};
    }

    const auto failure_at = [](PowerResult failure, const char* step) {
        failure.step = step;
        return failure;
    };

    const esp_err_t wake_error = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (wake_error != ESP_OK) {
        return {wake_error, static_cast<int>(wake_error), "esp_wakeup_disable"};
    }

    if (mode == L2Mode::imu_wake) {
        const esp_err_t reset_error = gpio_reset_pin(m5pm::board::kAggregateIrq);
        if (reset_error != ESP_OK) {
            return {reset_error, static_cast<int>(reset_error), "aggregate_irq_reset"};
        }
        esp_err_t gpio_error = rtc_gpio_deinit(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_deinit"};
        }
        gpio_error = rtc_gpio_init(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_init"};
        }
        gpio_error = rtc_gpio_set_direction(m5pm::board::kAggregateIrq, RTC_GPIO_MODE_INPUT_ONLY);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_direction"};
        }
        gpio_error = rtc_gpio_pulldown_dis(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_pulldown"};
        }
        gpio_error = rtc_gpio_pullup_en(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_pullup"};
        }
        gpio_error = esp_sleep_enable_ext0_wakeup(m5pm::board::kAggregateIrq, 0);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_ext0"};
        }
    }

    if (mode == L2Mode::rtc_alarm_wake || mode == L2Mode::rtc_wake) {
        // Clear any digital-domain pull, direction, or hold state left by a
        // previous boot before handing the aggregate IRQ pin to the RTC.
        const esp_err_t reset_error = gpio_reset_pin(m5pm::board::kAggregateIrq);
        if (reset_error != ESP_OK) {
            return {reset_error, static_cast<int>(reset_error), "aggregate_irq_reset"};
        }
        esp_err_t gpio_error = rtc_gpio_deinit(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_deinit"};
        }
        gpio_error = rtc_gpio_init(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_init"};
        }
        gpio_error = rtc_gpio_set_direction(m5pm::board::kAggregateIrq, RTC_GPIO_MODE_INPUT_ONLY);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_direction"};
        }
        gpio_error = rtc_gpio_pulldown_dis(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_pulldown"};
        }
        gpio_error = rtc_gpio_pullup_en(m5pm::board::kAggregateIrq);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_pullup"};
        }
        gpio_error = esp_sleep_enable_ext0_wakeup(m5pm::board::kAggregateIrq, 0);
        if (gpio_error != ESP_OK) {
            return {gpio_error, static_cast<int>(gpio_error), "aggregate_irq_ext0"};
        }
    }

    if (mode == L2Mode::button_wake) {
        constexpr std::uint64_t kButtonMask = (1ULL << m5pm::board::kKey1) | (1ULL << m5pm::board::kKey2);
        constexpr gpio_num_t kButtonPins[]  = {m5pm::board::kKey1, m5pm::board::kKey2};
        for (const gpio_num_t pin : kButtonPins) {
            esp_err_t gpio_error = gpio_reset_pin(pin);
            if (gpio_error != ESP_OK) {
                return {gpio_error, static_cast<int>(gpio_error), "button_gpio_reset"};
            }
            gpio_error = rtc_gpio_deinit(pin);
            if (gpio_error != ESP_OK) {
                return {gpio_error, static_cast<int>(gpio_error), "button_rtc_deinit"};
            }
            gpio_error = rtc_gpio_init(pin);
            if (gpio_error != ESP_OK) {
                return {gpio_error, static_cast<int>(gpio_error), "button_rtc_init"};
            }
            gpio_error = rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
            if (gpio_error != ESP_OK) {
                return {gpio_error, static_cast<int>(gpio_error), "button_rtc_direction"};
            }
            // Keep the RTC pull-up disabled; the external button circuit defines
            // the idle level and the EXT1 wake condition remains active-low.
            gpio_error = rtc_gpio_pulldown_dis(pin);
            if (gpio_error != ESP_OK) {
                return {gpio_error, static_cast<int>(gpio_error), "button_rtc_pulldown"};
            }
        }
        const esp_err_t ext1_error = esp_sleep_enable_ext1_wakeup(kButtonMask, ESP_EXT1_WAKEUP_ANY_LOW);
        if (ext1_error != ESP_OK) {
            return {ext1_error, static_cast<int>(ext1_error), "button_ext1"};
        }
    }

    PowerResult step = {};
#if M5PM_POWER_HAS_IOE1
    // Keep the no-wake L2 path's IOE1 internal pull disabled to match the
    // board's measured low-current state. Wake paths retain their
    // mode-specific pull policy for isolated validation.
    const m5ioe1_pull_config_t ioe1_pull = (mode == L2Mode::no_wake || mode == L2Mode::button_wake ||
                                            mode == L2Mode::rtc_alarm_wake || mode == L2Mode::rtc_wake)
                                               ? M5IOE1_PULL_DISABLED
                                               : M5IOE1_PULL_ENABLED;
    step = result(ioe1_.setI2cConfig(1, M5IOE1_I2C_SPEED_100K, M5IOE1_WAKE_EDGE_RISING, ioe1_pull));
    if (!step.ok()) {
        return failure_at(step, "ioe1_idle_sleep");
    }
#else
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#endif

    // RX8130 timer IRQ is a short pulse. Keep PM1 I2C awake on the RTC wake
    // path so GPIO0 can capture and forward that pulse reliably; the timer
    // test relies on this behavior instead of a latched level.
    if (mode != L2Mode::imu_wake && mode != L2Mode::rtc_wake) {
        step = result(pm1_.setI2cSleepTime(1));
        if (!step.ok()) {
            return failure_at(step, "pm1_idle_sleep");
        }
    }

    constexpr struct {
        gpio_num_t pin;
        const char* step;
    } kAlwaysIsolatedPins[] = {
        {m5pm::board::kTouchIrq, "touch_irq_isolate"},
        {m5pm::board::kImuIrq, "imu_irq_isolate"},
        {m5pm::board::kLoraIrq, "lora_irq_isolate"},
    };
    for (const auto& pin_config : kAlwaysIsolatedPins) {
        const esp_err_t isolate_error = rtc_gpio_isolate(pin_config.pin);
        if (isolate_error != ESP_OK) {
            return {isolate_error, static_cast<int>(isolate_error), pin_config.step};
        }
    }
    if (mode != L2Mode::button_wake) {
        constexpr struct {
            gpio_num_t pin;
            const char* step;
        } kButtonPins[] = {
            {m5pm::board::kKey1, "key1_isolate"},
            {m5pm::board::kKey2, "key2_isolate"},
        };
        for (const auto& pin_config : kButtonPins) {
            const esp_err_t isolate_error = rtc_gpio_isolate(pin_config.pin);
            if (isolate_error != ESP_OK) {
                return {isolate_error, static_cast<int>(isolate_error), pin_config.step};
            }
        }
    }
    if (mode == L2Mode::no_wake) {
        const esp_err_t isolate_error = rtc_gpio_isolate(m5pm::board::kAggregateIrq);
        if (isolate_error != ESP_OK) {
            return {isolate_error, static_cast<int>(isolate_error), "aggregate_irq_isolate"};
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_deep_sleep_start();
}

PowerResult PowerController::enter_l2()
{
    PowerResult step = prepare_l2(L2Mode::no_wake);
    if (!step.ok()) {
        return step;
    }
    return finish_l2(L2Mode::no_wake);
}

PowerResult PowerController::prepare_awake_l2()
{
    return prepare_l2(L2Mode::no_wake);
}

PowerResult PowerController::restore_display()
{
#if !M5PM_POWER_HAS_IOE1
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    if (!l2_prepared_ || l2_mode_ != L2Mode::no_wake || !ioe1_initialized_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "awake_l2_state"};
    }

    m5ioe1_err_t ioe_error = M5IOE1_OK;
    ioe1_.pinModeWithRes(M5IOE1_PIN_3, OUTPUT, &ioe_error);
    PowerResult step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_enable_mode"};
    step = result(ioe1_.setDriveMode(M5IOE1_PIN_3, M5IOE1_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "display_enable_drive"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_3, HIGH, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_enable_high"};

    ioe1_.pinModeWithRes(M5IOE1_PIN_5, OUTPUT, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_reset_mode"};
    step = result(ioe1_.setDriveMode(M5IOE1_PIN_5, M5IOE1_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "display_reset_drive"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_5, LOW, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_reset_low"};
    vTaskDelay(pdMS_TO_TICKS(8));
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_5, HIGH, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_reset_high"};
    vTaskDelay(pdMS_TO_TICKS(2));
    return {};
#endif
}

PowerResult PowerController::restore_nfc()
{
#if !M5PM_POWER_HAS_IOE1
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    if (!l2_prepared_ || !ioe1_initialized_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "awake_l2_state"};
    }

    // The awake L2 baseline releases IOE1 GPIO4 and powers the ST25R3916 down.
    // Re-enable the board rail before the NFC driver performs its identity read.
    // The board HAL requires 120 ms for this rail to settle before ST25R3916
    // initialization starts; a shorter delay can brown out when the RF field is enabled.
    m5ioe1_err_t ioe_error = M5IOE1_OK;
    ioe1_.pinModeWithRes(M5IOE1_PIN_4, OUTPUT, &ioe_error);
    PowerResult step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "nfc_enable_mode"};
    step = result(ioe1_.setDriveMode(M5IOE1_PIN_4, M5IOE1_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "nfc_enable_drive"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_4, HIGH, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "nfc_enable_high"};
    vTaskDelay(pdMS_TO_TICKS(120));
    return {};
#endif
}

PowerResult PowerController::set_frontlight(std::uint8_t percent)
{
    if (!l2_prepared_ || percent > 100) {
        return {ESP_ERR_INVALID_ARG, M5PM1_ERR_INVALID_ARG, "frontlight_argument"};
    }
    PowerResult step = result(pm1_.gpioSetFunc(M5PM1_GPIO_NUM_3, M5PM1_GPIO_FUNC_OTHER));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_function"};
    // The L2 baseline leaves PM1 GPIO3 as an input/open-drain safety state.
    // PWM output also requires the pin direction and drive state used by the
    // reference project's pm1.pinMode(PYG3_BL_PWM, M5PM1_OTHER) call.
    step = result(pm1_.gpioSetMode(M5PM1_GPIO_NUM_3, M5PM1_GPIO_MODE_OUTPUT));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_mode"};
    step = result(pm1_.gpioSetPull(M5PM1_GPIO_NUM_3, M5PM1_GPIO_PULL_NONE));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_pull"};
    step = result(pm1_.gpioSetDrive(M5PM1_GPIO_NUM_3, M5PM1_GPIO_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_drive"};
    step = result(pm1_.setPwmFrequency(5000));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_frequency"};
    step = result(pm1_.setPwmDuty(M5PM1_PWM_CH_0, percent, false, percent != 0));
    if (!step.ok()) return {step.error, step.raw_error, "frontlight_duty"};
    return {};
}

PowerResult PowerController::power_off_display()
{
#if !M5PM_POWER_HAS_IOE1
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    PowerResult step = set_frontlight(0);
    if (!step.ok()) return step;
    m5ioe1_err_t ioe_error = M5IOE1_OK;
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_5, LOW, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_reset_low"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_3, LOW, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "display_enable_low"};
    return {};
#endif
}

PowerResult PowerController::restore_imu()
{
    if (!l2_prepared_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "awake_l2_state"};
    }
    PowerResult step = result(pm1_.gpioSetFunc(M5PM1_GPIO_NUM_4, M5PM1_GPIO_FUNC_GPIO));
    if (!step.ok()) return {step.error, step.raw_error, "imu_irq_function"};
    step = result(
        pm1_.gpioSet(M5PM1_GPIO_NUM_4, M5PM1_GPIO_MODE_INPUT, 0, M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "imu_irq_input"};
    return {};
}

PowerResult PowerController::restore_lora()
{
#if !M5PM_POWER_HAS_IOE1
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    if (!l2_prepared_ || !ioe1_initialized_) {
        return {ESP_ERR_INVALID_STATE, M5PM1_ERR_NOT_INIT, "awake_l2_state"};
    }
    PowerResult step = result(pm1_.gpioSetFunc(M5PM1_GPIO_NUM_2, M5PM1_GPIO_FUNC_GPIO));
    if (!step.ok()) return {step.error, step.raw_error, "lora_enable_function"};
    step = result(
        pm1_.gpioSet(M5PM1_GPIO_NUM_2, M5PM1_GPIO_MODE_OUTPUT, 1, M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "lora_enable_high"};

    m5ioe1_err_t ioe_error             = M5IOE1_OK;
    constexpr std::uint8_t kLoraPins[] = {M5IOE1_PIN_2, M5IOE1_PIN_10};
    for (const std::uint8_t pin : kLoraPins) {
        ioe1_.pinModeWithRes(pin, OUTPUT, &ioe_error);
        step = result(ioe_error);
        if (!step.ok()) return {step.error, step.raw_error, "lora_ioe_mode"};
        step = result(ioe1_.setDriveMode(pin, M5IOE1_DRIVE_PUSHPULL));
        if (!step.ok()) return {step.error, step.raw_error, "lora_ioe_drive"};
    }
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_2, HIGH, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "lora_antenna_high"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_10, LOW, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "lora_reset_low"};
    vTaskDelay(pdMS_TO_TICKS(100));
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_10, HIGH, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "lora_reset_high"};
    vTaskDelay(pdMS_TO_TICKS(20));
    return {};
#endif
}

PowerResult PowerController::power_off_lora()
{
#if !M5PM_POWER_HAS_IOE1
    return {ESP_ERR_NOT_SUPPORTED, M5PM1_ERR_NOT_SUPPORTED, "ioe1_unavailable"};
#else
    m5ioe1_err_t ioe_error = M5IOE1_OK;
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_10, LOW, &ioe_error);
    PowerResult step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "lora_reset_low"};
    ioe1_.digitalWriteWithRes(M5IOE1_PIN_2, LOW, &ioe_error);
    step = result(ioe_error);
    if (!step.ok()) return {step.error, step.raw_error, "lora_antenna_low"};
    step = result(
        pm1_.gpioSet(M5PM1_GPIO_NUM_2, M5PM1_GPIO_MODE_OUTPUT, 0, M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_PUSHPULL));
    if (!step.ok()) return {step.error, step.raw_error, "lora_enable_low"};
    return {};
#endif
}

PowerResult PowerController::prepare_imu_wake_l2()
{
    return prepare_l2(L2Mode::imu_wake);
}

PowerResult PowerController::enter_imu_wake_l2()
{
    return finish_l2(L2Mode::imu_wake);
}

PowerResult PowerController::enter_button_wake_l2()
{
    PowerResult step = prepare_l2(L2Mode::button_wake);
    if (!step.ok()) {
        return step;
    }
    return finish_l2(L2Mode::button_wake);
}

PowerResult PowerController::prepare_rtc_alarm_wake_l2()
{
    return prepare_l2(L2Mode::rtc_alarm_wake);
}

PowerResult PowerController::enter_rtc_alarm_wake_l2()
{
    return finish_l2(L2Mode::rtc_alarm_wake);
}

PowerResult PowerController::prepare_rtc_wake_l2()
{
    return prepare_l2(L2Mode::rtc_wake);
}

PowerResult PowerController::enter_rtc_wake_l2()
{
    return finish_l2(L2Mode::rtc_wake);
}

}  // namespace m5pm::power
