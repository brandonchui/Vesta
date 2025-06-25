#pragma once

#ifdef ENABLE_DEBUG

// --- SPDLOG CONFIGURATION ---
#define SPDLOG_HEADER_ONLY
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
// --- END SPDLOG CONFIGURATION ---

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include "../Config.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
    static std::shared_ptr<spdlog::logger> g_game_logger;
}

// Initializes the logging system with a console sink.
inline void InitLogging(const char* logger_name = "VT_APP") {
    try {
        std::vector<spdlog::sink_ptr> sinks;

        // 1. Console Sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
        sinks.push_back(console_sink);

        g_game_logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
        g_game_logger->set_level(spdlog::level::trace);
        g_game_logger->flush_on(spdlog::level::trace);

        if (g_game_logger) {
            // g_game_logger->info("Console logging system initialized.");
        } else {
            printf("CRITICAL LOGGER ERROR: null\n");
        }

    } catch (const spdlog::spdlog_ex& ex) {
        printf("Log initialization failed: %s\n", ex.what());
        g_game_logger.reset();
    }
}

inline void ShutdownLogging() {
    if (g_game_logger) {
        // g_game_logger->info("Logging system shutting down.");
        g_game_logger->flush();
    }
    spdlog::shutdown();
    g_game_logger.reset();
}

// --- Formatted Logging Functions (Templates) ---
template <typename... Args> void LogTraceFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (g_game_logger) {
        g_game_logger->trace(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args> void LogInfoFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (g_game_logger) {
        g_game_logger->info(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args> void LogWarnFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (g_game_logger) {
        g_game_logger->warn(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args> void LogErrorFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    if (g_game_logger) {
        g_game_logger->error(fmt, std::forward<Args>(args)...);
    }
}

// --- Public-Facing Logging Macros ---
#define VT_TRACE(fmt, ...) LogTraceFmt(fmt, ##__VA_ARGS__)
#define VT_INFO(fmt, ...) LogInfoFmt(fmt, ##__VA_ARGS__)
#define VT_WARN(fmt, ...) LogWarnFmt(fmt, ##__VA_ARGS__)
#define VT_ERROR(fmt, ...) LogErrorFmt(fmt, ##__VA_ARGS__)

#else

// These macros expand to nothing in release mode
#define InitLogging(...) ((void) 0)
#define ShutdownLogging(...) ((void) 0)
#define VT_TRACE(...) ((void) 0)
#define VT_INFO(...) ((void) 0)
#define VT_WARN(...) ((void) 0)
#define VT_ERROR(...) ((void) 0)

#endif // ENABLE_DEBUG
