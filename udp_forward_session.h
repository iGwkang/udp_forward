#pragma once

#include <asio.hpp>

#include "common.h"

class UDPForwardSession : public std::enable_shared_from_this<UDPForwardSession>
{
public:
    UDPForwardSession(asio::ip::udp::socket & li_udp_socket, asio::ip::udp::socket dst_udp_socket, const asio::ip::udp::endpoint & src_endpoint, const asio::ip::udp::endpoint & dst_endpoint);
    ~UDPForwardSession();

    std::function<void()> close_handle;

    void do_read_data();

    void on_udp_data(std::vector<uint8_t> && data);

    std::chrono::steady_clock::time_point last_alive_timepoint() const;

    void close();

private:
    void on_close();

private:
    asio::ip::udp::socket & m_li_udp_socket;
    asio::ip::udp::socket   m_dst_udp_socket;
    asio::ip::udp::endpoint m_src_endpoint;
    asio::ip::udp::endpoint m_dst_endpoint;

    std::vector<uint8_t> m_recv_buffer;
    std::chrono::steady_clock::time_point m_last_alive_time_point;
};
