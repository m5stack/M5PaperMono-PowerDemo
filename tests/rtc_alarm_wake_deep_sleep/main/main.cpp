#include "m5pm_rtc_test.hpp"

extern "C" void app_main(void)
{
    m5pm::rtc_test::run("rtc_alarm_deep", "rtc_alarm_wake_deep_sleep", m5pm::rtc_test::Event::alarm, false);
}
