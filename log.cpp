#include "common.h"

#include <signal.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif

#if _WIN32
#include <Windows.h>
#endif

//static void signal_handler(int signal)
//{
//    LOG_ERROR("recv signal {}", signal);
//    spdlog::dump_backtrace();
//}

void init_log(const char *logPath, spdlog::level::level_enum logLevel)
{
#if _WIN32
    AttachConsole(ATTACH_PARENT_PROCESS);
#endif
    spdlog::default_logger()->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [%t] [%s:%#] %v");
    try
    {
        if (logPath && strcmp(logPath, "") != 0)
        {
            //std::shared_ptr<spdlog::logger> logger = spdlog::basic_logger_mt("logger", logPath);
            auto logger = spdlog::daily_logger_mt("logger", logPath);
            logger->enable_backtrace(20);
            logger->set_pattern("[%Y-%m-%d %T.%e] [%l] [%t] [%s:%#] [%!] %v");
            spdlog::set_default_logger(logger);
            spdlog::flush_every(std::chrono::seconds(3));
        }
#ifdef __ANDROID__
        else
        {
            std::shared_ptr<spdlog::logger> android_logger = spdlog::android_logger_mt("android-logger");
            spdlog::set_default_logger(android_logger);
        }
#endif
    }
    catch (const std::exception & e)
    {
        LOG_WARN("init logger failure, {}", e.what());
    }

    spdlog::default_logger()->set_level(logLevel);

    //signal(SIGABRT, signal_handler);
    //signal(SIGFPE, signal_handler);
    //signal(SIGILL, signal_handler);
    //signal(SIGINT, signal_handler);
    //signal(SIGSEGV, signal_handler);
    //signal(SIGTERM, signal_handler);
}
