#pragma once

#ifdef ENABLE_LOG

#include <fmt/chrono.h>
#include <fmt/core.h>

inline std::string GetLocalTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

#define LOG_INFO(...) \
    fmt::print("[INFO] [{}:{}] {} {}\n", __FILE__, __LINE__, GetLocalTimeString(), fmt::format(__VA_ARGS__))

#define LOG_DEBUG(...) \
    fmt::print("[DEBUG] [{}:{}] {} {}\n", __FILE__, __LINE__, GetLocalTimeString(), fmt::format(__VA_ARGS__))

#define LOG_ERROR(...) \
    fmt::print("[ERROR] [{}:{}] {} {}\n", __FILE__, __LINE__, GetLocalTimeString(), fmt::format(__VA_ARGS__))

#else

#define LOG_INFO(...) ((void) 0)

#define LOG_DEBUG(...) ((void) 0)

#define LOG_ERROR(...) ((void) 0)

#endif
