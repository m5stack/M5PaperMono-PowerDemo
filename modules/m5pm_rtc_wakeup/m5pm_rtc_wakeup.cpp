#include "m5pm_rtc_wakeup.hpp"

#include <array>

#include "m5pm_board.hpp"

namespace m5pm::wakeup {
namespace {

constexpr std::uint8_t kRegSeconds   = 0x10;
constexpr std::uint8_t kRegAlarmMin  = 0x17;
constexpr std::uint8_t kRegTimerLow  = 0x1A;
constexpr std::uint8_t kRegExtension = 0x1C;
constexpr std::uint8_t kRegFlag      = 0x1D;
constexpr std::uint8_t kRegControl0  = 0x1E;

constexpr std::uint8_t kExtensionTimerEnable = 1U << 4;
constexpr std::uint8_t kExtensionTimerSelect = 0x07U;
constexpr std::uint8_t kControlAlarmEnable   = 1U << 3;
constexpr std::uint8_t kControlTimerEnable   = 1U << 4;
constexpr std::uint8_t kControlUpdateEnable  = 1U << 5;
constexpr std::uint8_t kControlStop          = 1U << 6;
constexpr std::uint8_t kControlIrqMask       = kControlAlarmEnable | kControlTimerEnable | kControlUpdateEnable;

// 2025-09-27 12:00:00, Saturday. RX8130 stores weekday as a one-hot mask.
constexpr std::array<std::uint8_t, 7> kButtonBaseline = {
    0x00, 0x00, 0x12, 1U << 6, 0x27, 0x09, 0x25,
};

}  // namespace

RtcWakeup::~RtcWakeup()
{
    if (device_ != nullptr) {
        i2c_bus_device_delete(&device_);
    }
}

RtcResult RtcWakeup::begin(i2c_bus_handle_t bus, std::uint32_t speed_hz)
{
    if (bus == nullptr || (speed_hz != 100000U && speed_hz != 400000U)) {
        return {ESP_ERR_INVALID_ARG, ESP_ERR_INVALID_ARG, "rtc_begin_argument"};
    }
    if (device_ != nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "rtc_begin_state"};
    }

    device_ = i2c_bus_device_create(bus, m5pm::board::kRtcAddress, speed_hz);
    if (device_ == nullptr) {
        return {ESP_ERR_NO_MEM, ESP_ERR_NO_MEM, "rtc_device_create"};
    }

    std::uint8_t probe = 0;
    RtcResult result   = read(kRegSeconds, &probe, 1, "rtc_probe");
    if (!result.ok()) {
        i2c_bus_device_delete(&device_);
    }
    return result;
}

RtcResult RtcWakeup::read(std::uint8_t reg, std::uint8_t* data, std::size_t length, const char* step) const
{
    if (device_ == nullptr || data == nullptr || length == 0U) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, step};
    }
    const esp_err_t error = i2c_bus_read_bytes(device_, reg, length, data);
    return {error, static_cast<int>(error), step};
}

RtcResult RtcWakeup::write(std::uint8_t reg, const std::uint8_t* data, std::size_t length, const char* step)
{
    if (device_ == nullptr || data == nullptr || length == 0U) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, step};
    }
    const esp_err_t error = i2c_bus_write_bytes(device_, reg, length, data);
    return {error, static_cast<int>(error), step};
}

RtcResult RtcWakeup::read_flags(RtcFlags* flags) const
{
    if (flags == nullptr) {
        return {ESP_ERR_INVALID_ARG, ESP_ERR_INVALID_ARG, "rtc_flags_argument"};
    }
    return read(kRegFlag, &flags->raw, 1, "rtc_flags_read");
}

RtcResult RtcWakeup::configure_button_baseline()
{
    if (device_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "rtc_baseline_state"};
    }

    std::uint8_t control = 0;
    RtcResult result     = write(kRegControl0, &control, 1, "rtc_interrupt_disable");
    if (!result.ok()) {
        return result;
    }

    std::uint8_t extension = 0;
    result                 = read(kRegExtension, &extension, 1, "rtc_extension_read");
    if (!result.ok()) {
        return result;
    }
    extension = static_cast<std::uint8_t>(extension & ~kExtensionTimerEnable);
    result    = write(kRegExtension, &extension, 1, "rtc_timer_disable");
    if (!result.ok()) {
        return result;
    }

    constexpr std::uint8_t kClearFlags = 0;
    result                             = write(kRegFlag, &kClearFlags, 1, "rtc_flags_clear");
    if (!result.ok()) {
        return result;
    }

    const std::uint8_t stopped_control = static_cast<std::uint8_t>(control | kControlStop);
    result                             = write(kRegControl0, &stopped_control, 1, "rtc_clock_stop");
    if (!result.ok()) {
        return result;
    }
    result = write(kRegSeconds, kButtonBaseline.data(), kButtonBaseline.size(), "rtc_time_set");
    if (!result.ok()) {
        return result;
    }

    std::array<std::uint8_t, kButtonBaseline.size()> time_readback = {};
    result = read(kRegSeconds, time_readback.data(), time_readback.size(), "rtc_time_readback");
    if (!result.ok()) {
        return result;
    }
    if (time_readback != kButtonBaseline) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "rtc_time_verify"};
    }

    control = static_cast<std::uint8_t>(control & ~kControlStop);
    result  = write(kRegControl0, &control, 1, "rtc_clock_start");
    if (!result.ok()) {
        return result;
    }

    std::uint8_t verify_control = 0;
    result                      = read(kRegControl0, &verify_control, 1, "rtc_control_readback");
    if (!result.ok()) {
        return result;
    }
    std::uint8_t verify_extension = 0;
    result                        = read(kRegExtension, &verify_extension, 1, "rtc_extension_readback");
    if (!result.ok()) {
        return result;
    }
    RtcFlags verify_flags = {};
    result                = read_flags(&verify_flags);
    if (!result.ok()) {
        return result;
    }
    const bool trigger_flag_latched = verify_flags.alarm() || verify_flags.timer();
    if ((verify_control & (kControlIrqMask | kControlStop)) != 0U || (verify_extension & kExtensionTimerEnable) != 0U ||
        trigger_flag_latched) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "rtc_baseline_verify"};
    }
    return {};
}

RtcResult RtcWakeup::configure_alarm_120s()
{
    RtcResult result = configure_button_baseline();
    if (!result.ok()) {
        return result;
    }

    // Make the alarm path independent from a timer configuration left by a
    // previous boot. Disable the timer in both registers and clear its
    // latched flag immediately before arming the calendar alarm.
    std::uint8_t extension = 0;
    result                 = read(kRegExtension, &extension, 1, "rtc_alarm_timer_extension_read");
    if (!result.ok()) {
        return result;
    }
    extension = static_cast<std::uint8_t>(extension & ~(kExtensionTimerEnable | kExtensionTimerSelect));
    result    = write(kRegExtension, &extension, 1, "rtc_alarm_timer_disable");
    if (!result.ok()) {
        return result;
    }

    std::uint8_t control = 0;
    result               = read(kRegControl0, &control, 1, "rtc_alarm_control_read");
    if (!result.ok()) {
        return result;
    }
    control = static_cast<std::uint8_t>(control & ~(kControlTimerEnable | kControlUpdateEnable | kControlStop));
    result  = write(kRegControl0, &control, 1, "rtc_alarm_timer_irq_disable");
    if (!result.ok()) {
        return result;
    }

    constexpr std::uint8_t kClearFlags = 0;
    result                             = write(kRegFlag, &kClearFlags, 1, "rtc_alarm_flags_clear");
    if (!result.ok()) {
        return result;
    }

    constexpr std::uint8_t kAlarmMinute       = 0x02;
    constexpr std::uint8_t kDisableAlarmField = 0x80;
    result                                    = write(kRegAlarmMin, &kAlarmMinute, 1, "rtc_alarm_minute");
    if (!result.ok()) {
        return result;
    }
    result = write(static_cast<std::uint8_t>(kRegAlarmMin + 1), &kDisableAlarmField, 1, "rtc_alarm_hour_disable");
    if (!result.ok()) {
        return result;
    }
    result = write(static_cast<std::uint8_t>(kRegAlarmMin + 2), &kDisableAlarmField, 1, "rtc_alarm_weekday_disable");
    if (!result.ok()) {
        return result;
    }
    result = write(kRegFlag, &kClearFlags, 1, "rtc_alarm_flag_clear");
    if (!result.ok()) {
        return result;
    }
    constexpr std::uint8_t kAlarmEnable = kControlAlarmEnable;
    result                              = write(kRegControl0, &kAlarmEnable, 1, "rtc_alarm_enable");
    if (!result.ok()) {
        return result;
    }

    std::uint8_t verify_extension = 0;
    result                        = read(kRegExtension, &verify_extension, 1, "rtc_alarm_timer_extension_verify");
    if (!result.ok()) {
        return result;
    }
    std::uint8_t verify_control = 0;
    result                      = read(kRegControl0, &verify_control, 1, "rtc_alarm_control_verify");
    if (!result.ok()) {
        return result;
    }
    RtcFlags verify_flags = {};
    result                = read_flags(&verify_flags);
    if (!result.ok()) {
        return result;
    }
    if ((verify_control & kControlAlarmEnable) == 0U || (verify_extension & kExtensionTimerEnable) != 0U ||
        (verify_control & (kControlTimerEnable | kControlUpdateEnable | kControlStop)) != 0U || verify_flags.timer()) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "rtc_alarm_timer_verify"};
    }
    return {};
}

RtcResult RtcWakeup::configure_timer_120s()
{
    RtcResult result = configure_button_baseline();
    if (!result.ok()) {
        return result;
    }

    constexpr std::uint16_t kTimerSeconds = 120U;
    constexpr std::uint8_t kTimerCount[2] = {static_cast<std::uint8_t>(kTimerSeconds & 0xFFU),
                                             static_cast<std::uint8_t>(kTimerSeconds >> 8U)};
    // RX8130 requires the timer to be stopped before loading a new counter.
    // Writing the counter while TE is set can leave the previous countdown
    // active and prevent the new 120-second wake event from being generated.
    std::uint8_t extension = 0;
    result                 = read(kRegExtension, &extension, 1, "rtc_timer_extension_read");
    if (!result.ok()) {
        return result;
    }
    extension = static_cast<std::uint8_t>(extension & ~kExtensionTimerEnable);
    result    = write(kRegExtension, &extension, 1, "rtc_timer_disable");
    if (!result.ok()) {
        return result;
    }
    // Prevent a calendar alarm left by a previous alarm test from latching at
    // the same time as the countdown timer. The flag can latch even when AIE
    // is disabled, so disable all alarm match fields explicitly.
    constexpr std::uint8_t kDisableAlarmField = 0x80;
    result = write(kRegAlarmMin, &kDisableAlarmField, 1, "rtc_timer_alarm_minute_disable");
    if (!result.ok()) {
        return result;
    }
    result = write(static_cast<std::uint8_t>(kRegAlarmMin + 1), &kDisableAlarmField, 1, "rtc_timer_alarm_hour_disable");
    if (!result.ok()) {
        return result;
    }
    result =
        write(static_cast<std::uint8_t>(kRegAlarmMin + 2), &kDisableAlarmField, 1, "rtc_timer_alarm_weekday_disable");
    if (!result.ok()) {
        return result;
    }
    result = write(kRegTimerLow, kTimerCount, sizeof(kTimerCount), "rtc_timer_count");
    if (!result.ok()) {
        return result;
    }
    result = read(kRegExtension, &extension, 1, "rtc_timer_extension_read");
    if (!result.ok()) {
        return result;
    }
    extension = static_cast<std::uint8_t>((extension & ~kExtensionTimerSelect) | 0x02U | kExtensionTimerEnable);
    result    = write(kRegExtension, &extension, 1, "rtc_timer_enable");
    if (!result.ok()) {
        return result;
    }
    constexpr std::uint8_t kClearFlags = 0;
    result                             = write(kRegFlag, &kClearFlags, 1, "rtc_timer_flag_clear");
    if (!result.ok()) {
        return result;
    }
    constexpr std::uint8_t kTimerEnable = kControlTimerEnable;
    result                              = write(kRegControl0, &kTimerEnable, 1, "rtc_timer_irq_enable");
    if (!result.ok()) {
        return result;
    }

    std::uint8_t verify_extension = 0;
    result                        = read(kRegExtension, &verify_extension, 1, "rtc_timer_extension_verify");
    if (!result.ok()) {
        return result;
    }
    std::uint8_t verify_control = 0;
    result                      = read(kRegControl0, &verify_control, 1, "rtc_timer_control_verify");
    if (!result.ok()) {
        return result;
    }
    std::uint8_t verify_count[2] = {};
    result                       = read(kRegTimerLow, verify_count, sizeof(verify_count), "rtc_timer_count_verify");
    if (!result.ok()) {
        return result;
    }
    const std::uint16_t count =
        static_cast<std::uint16_t>(verify_count[0]) | (static_cast<std::uint16_t>(verify_count[1]) << 8U);
    const bool timer_enabled = (verify_extension & kExtensionTimerEnable) != 0U;
    const bool timer_1hz     = (verify_extension & kExtensionTimerSelect) == 0x02U;
    const bool timer_irq     = (verify_control & kControlTimerEnable) != 0U;
    if (!timer_enabled || !timer_irq || !timer_1hz || count != kTimerSeconds) {
        return {ESP_ERR_INVALID_RESPONSE, ESP_ERR_INVALID_RESPONSE, "rtc_timer_verify"};
    }
    return {};
}

RtcResult RtcWakeup::release_bus()
{
    if (device_ == nullptr) {
        return {ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, "rtc_release_state"};
    }
    const esp_err_t error = i2c_bus_device_delete(&device_);
    return {error, static_cast<int>(error), "rtc_device_delete"};
}

}  // namespace m5pm::wakeup
