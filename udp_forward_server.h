#pragma once

#include <asio.hpp>

#include "common.h"
#include "udp_forward_session.h"

class UDPForwardServer
{
    using session_map_t = std::unordered_map<asio::ip::udp::endpoint, std::weak_ptr<UDPForwardSession>>;

public:
    UDPForwardServer(asio::io_context & io_context, const std::string & forward_to_ip, uint16_t forward_to_port, uint32_t alive_timeout = 5);
    ~UDPForwardServer();

    bool listen(const std::string & eth_ip = "", uint16_t udp_port = 0);

    void start();

private:
    void do_read_data();
    void do_read_data(asio::ip::udp::socket & udp_socket, std::vector<uint8_t> & udp_read_buffer);
    void do_check_connection_alive();

private:
    struct bind_socket_t
    {
        asio::ip::udp::socket udp_socket;
        std::vector<uint8_t>  udp_read_buffer;
    };

    asio::io_context &                  m_io_context; //{ ASIO_CONCURRENCY_HINT_UNSAFE };
    std::chrono::seconds                m_alive_timeout{ 0 };
    std::unique_ptr<asio::steady_timer> m_alive_timer{};

    asio::ip::udp::endpoint    m_forward_to_endpoint{};
    std::vector<bind_socket_t> m_bind_socket_vec{};
    session_map_t              m_udp_sessions{};
};
