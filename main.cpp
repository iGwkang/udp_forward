#if __has_include(<gnu/libc-version.h>)
#include <gnu/libc-version.h>
#endif

#if __linux__
#include <sys/resource.h>
#endif

#include <regex>

#include <inipp.h>

#include "udp_forward_server.h"

static std::string log_level_str = "info";
static std::string log_path;
static std::string forward_config_path;
static uint32_t    alive_timeout = 5;

struct ip_with_port_t
{
    std::string ip{};
    int         port{};
};

static void cli(int argc, char ** argv)
{
    int n = 0;
    while (++n < argc)
    {
        if (0 == strcmp("-h", argv[n]))
        {
#ifdef __GIT_SHA1__
            printf("git last commit sha1: %s\n", __GIT_SHA1__);
#endif

#ifdef __GLIBC__
            printf("glibc version: %s(%s)\n", gnu_get_libc_version(), gnu_get_libc_release());
#endif

#if defined(__clang_major__) && defined(__clang_minor__)
            printf("clang version: %s\n", __clang_version__);
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
            printf("GCC version: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
            printf(R"(Usage: %s [-switches]
Available switches:
  -h                - display this help
  -c<config>        - forward configuration file path
  -l<log>           - log output path (default: console)
  -ll<log_level>    - log level (defaule:"%s", "trace", "debug", "info", "warning", "error", "critical", "off")
  -t<timeout>       - udp connection timeout in seconds (default: %u)
)",
                   argv[0], log_level_str.c_str(), alive_timeout);
            fflush(stdout);
            std::exit(EXIT_SUCCESS);
        }
        else if (0 == strcmp("-c", argv[n]) || 0 == strcmp("-config", argv[n]))
        {
            forward_config_path = argv[++n];
        }
        else if (0 == strcmp("-l", argv[n]) || 0 == strcmp("-log", argv[n]))
        {
            log_path = argv[++n];
        }
        else if (0 == strcmp("-ll", argv[n]) || 0 == strcmp("-log_level", argv[n]))
        {
            log_level_str = argv[++n];
        }
    }

    if (forward_config_path.empty())
    {
        LOG_ERROR("forward configuration file path is empty");
        std::exit(EXIT_FAILURE);
    }
}

static std::map<int, ip_with_port_t> read_forward_config(const std::string & path)
{
    inipp::Ini<char> ini;
    std::ifstream    is(path);
    ini.parse(is);

    std::regex  re("(\\d+\\.\\d+\\.\\d+\\.\\d+):(\\d+)");
    std::smatch sm;

    std::map<int, ip_with_port_t> forward_map;
    for (auto & [port, ip_with_port] : ini.sections["Forward"])
    {
        ip_with_port_t addr;
        if (std::regex_match(ip_with_port, sm, re))
        {
            addr.ip = sm[1].str();
            addr.port = std::stoi(sm[2].str());
            LOG_INFO("forward to ip: {}, port: {}", addr.ip, port);
        }
        else
        {
            LOG_ERROR("invalid forward to address format: {}", ip_with_port);
            continue;
        }

        forward_map[std::atoi(port.c_str())] = addr;
    }
    return forward_map;
}

int main(int argc, char * argv[])
{
    cli(argc, argv);

    init_log(log_path.c_str(), spdlog::level::from_str(log_level_str));

    auto forward_map = read_forward_config(forward_config_path);
#if __linux__
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    struct rlimit64 limit = {};
    for (int i = 30; i >= 10; --i)
    {
        limit.rlim_cur = limit.rlim_max = 1 << i;
        if (setrlimit64(RLIMIT_NOFILE, &limit) == 0)
            break;
    }
    if (getrlimit64(RLIMIT_NOFILE, &limit) == 0)
    {
        LOG_DEBUG("current process max open files limits {}", limit.rlim_cur);
    }
#endif

    std::vector<std::thread>                       thread_group;
    auto                                           thread_num = std::min<size_t>(std::thread::hardware_concurrency(), forward_map.size());
    std::vector<std::unique_ptr<asio::io_context>> io_contexts;
    io_contexts.reserve(thread_num);

    auto unit_cout = forward_map.size() / thread_num;
    if (forward_map.size() % thread_num != 0)
        ++thread_num;

    for (int i = 0; i < thread_num; ++i)
    {
        io_contexts.emplace_back(std::make_unique<asio::io_context>(ASIO_CONCURRENCY_HINT_UNSAFE));

        thread_group.emplace_back([&, i] {
            auto begin = forward_map.begin();
            auto end = forward_map.begin();
            std::advance(begin, unit_cout * i);
            std::advance(end, std::min<size_t>(unit_cout * i + unit_cout, forward_map.size()));

            std::vector<std::unique_ptr<UDPForwardServer>> forward_servers;

            for (auto iter = begin; iter != end; ++iter)
            {
                std::unique_ptr<UDPForwardServer> forward_server =
                    std::make_unique<UDPForwardServer>(*io_contexts[i], iter->second.ip, iter->second.port, alive_timeout);
                if (forward_server->listen("", iter->first))
                {
                    forward_server->start();
                    forward_servers.emplace_back(std::move(forward_server));
                }
            }

            std::error_code ec;
            io_contexts[i]->run(ec);
            LOG_ERROR_IF(ec, "io_context run error:{}", ec.message());
        });
    }

    for (auto & t : thread_group)
    {
        t.join();
    }

    return 0;
}
