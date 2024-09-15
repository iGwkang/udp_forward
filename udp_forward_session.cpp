#include "udp_forward_session.h"

UDPForwardSession::UDPForwardSession(asio::ip::udp::socket & li_udp_socket, asio::ip::udp::socket dst_udp_socket,
                                     const asio::ip::udp::endpoint & src_endpoint, const asio::ip::udp::endpoint & dst_endpoint)
    : m_li_udp_socket(li_udp_socket)
    , m_dst_udp_socket(std::move(dst_udp_socket))
    , m_src_endpoint(src_endpoint)
    , m_dst_endpoint(dst_endpoint)
    , m_recv_buffer(4 * 1024)
{}

UDPForwardSession::~UDPForwardSession() {}

void UDPForwardSession::do_read_data()
{
    auto self(shared_from_this());
    m_dst_udp_socket.async_receive(asio::buffer(m_recv_buffer), [=](const asio::error_code & ec, std::size_t bytes_transferred) {
        LAMBDA_REF(self);
        if (ec)
        {
            std::error_code local_endpoint_ec;
            LOG_WARN("udp socket {} receive from {} error code:{}, msg:{}", fmt::streamed(m_dst_udp_socket.local_endpoint(local_endpoint_ec)), fmt::streamed(m_dst_endpoint), ec.value(),
                     ec.message());
            on_close();
            return;
        }

        LOG_TRACE("recv udp data from {} to {}", fmt::streamed(m_dst_endpoint), fmt::streamed(m_src_endpoint));

        m_li_udp_socket.async_send_to(asio::buffer(m_recv_buffer.data(), bytes_transferred), m_src_endpoint,
                                      [=](const asio::error_code & ec, std::size_t bytes_transferred) {
                                          LAMBDA_REF(self);
                                          if (ec)
                                          {
                                              LOG_WARN("udp socket sendto {} error:{}", fmt::streamed(m_src_endpoint), ec.message());
                                              on_close();
                                              return;
                                          }
                                          m_last_alive_time_point = std::chrono::steady_clock::now();
                                          do_read_data();
                                      });
    });
}

void UDPForwardSession::on_udp_data(std::vector<uint8_t> && data)
{
    m_last_alive_time_point = std::chrono::steady_clock::now();
    auto self(shared_from_this());
    auto asio_buffer = asio::buffer(data);

    LOG_TRACE("recv udp data from {} to {}, size:{}", fmt::streamed(m_src_endpoint), fmt::streamed(m_dst_endpoint), data.size());

    m_dst_udp_socket.async_send(asio_buffer, [=, data = std::move(data)](const asio::error_code & ec, std::size_t bytes_transferred) {
        LAMBDA_REF(self);
        LOG_WARN_IF(ec, "udp socket sendto {} error:{}", fmt::streamed(m_dst_endpoint), ec.message());
    });
}

std::chrono::steady_clock::time_point UDPForwardSession::last_alive_timepoint() const
{
    return m_last_alive_time_point;
}

void UDPForwardSession::close()
{
    on_close();
}

void UDPForwardSession::on_close()
{
    if (m_dst_udp_socket.is_open())
    {
        std::error_code ec;
        m_dst_udp_socket.shutdown(m_dst_udp_socket.shutdown_both, ec);
        m_dst_udp_socket.close(ec);

        if (close_handle)
            close_handle();
    }
}
