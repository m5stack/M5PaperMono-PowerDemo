#include "m5pm_rtc_test.hpp"

extern "C" void app_main(void)
{
    m5pm::rtc_test::run("rtc_alarm_shutdown", "rtc_alarm_wake_shutdown", m5pm::rtc_test::Event::alarm, true);
}
