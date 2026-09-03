#include "m5pm_rtc_test.hpp"

extern "C" void app_main(void)
{
    m5pm::rtc_test::run("rtc_timer_shutdown", "rtc_timer_wake_shutdown", m5pm::rtc_test::Event::timer, true);
}
