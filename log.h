#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

void init_log(const char * logPath, spdlog::level::level_enum logLevel);


#define LOG_TRACE     SPDLOG_TRACE
#define LOG_TRACE_IF(condition, ...) !(condition) ? (void)0 : LOG_TRACE(__VA_ARGS__)

#define LOG_DEBUG     SPDLOG_DEBUG
#define LOG_DEBUG_IF(condition, ...) !(condition) ? (void)0 : LOG_DEBUG(__VA_ARGS__)

#define LOG_INFO      SPDLOG_INFO
#define LOG_INFO_IF(condition, ...) !(condition) ? (void)0 : LOG_INFO(__VA_ARGS__)

#define LOG_WARN      SPDLOG_WARN
#define LOG_WARN_IF(condition, ...) !(condition) ? (void)0 : LOG_WARN(__VA_ARGS__)

#define LOG_ERROR     SPDLOG_ERROR
#define LOG_ERROR_IF(condition, ...) !(condition) ? (void)0 : LOG_ERROR(__VA_ARGS__)

#define LOG_CRITICAL  SPDLOG_CRITICAL
#define LOG_CRITICAL_IF(condition, ...) !(condition) ? (void)0 : LOG_CRITICAL(__VA_ARGS__)


#define LOG_DEBUG_ONCE(...) static bool STRINGCAT(_once_log_, __LINE__) = (LOG_DEBUG(__VA_ARGS__), true); (void)(STRINGCAT(_once_log_, __LINE__))

#define LOG_INFO_ONCE(...) static bool STRINGCAT(_once_log_, __LINE__) = (LOG_INFO(__VA_ARGS__), true); (void)(STRINGCAT(_once_log_, __LINE__))

#define LOG_WARN_ONCE(...) static bool STRINGCAT(_once_log_, __LINE__) = (LOG_WARN(__VA_ARGS__), true); (void)(STRINGCAT(_once_log_, __LINE__))

#define LOG_ERROR_ONCE(...) static bool STRINGCAT(_once_log_, __LINE__) = (LOG_ERROR(__VA_ARGS__), true); (void)(STRINGCAT(_once_log_, __LINE__))

#define LOG_CRITICAL_ONCE(...) static bool STRINGCAT(_once_log_, __LINE__) = (LOG_CRITICAL(__VA_ARGS__), true); (void)(STRINGCAT(_once_log_, __LINE__))
