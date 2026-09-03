#pragma once

#include <cstdint>

namespace m5pm::rtc_test {

enum class Event : std::uint8_t {
    alarm,
    timer,
};

void run(const char* tag, const char* test_id, Event event, bool shutdown);

}  // namespace m5pm::rtc_test
