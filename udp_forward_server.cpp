#include "udp_forward_server.h"

UDPForwardServer::UDPForwardServer(asio::io_context & io_context, const std::string & forward_to_ip, uint16_t forward_to_port, uint32_t alive_timeout)
    : m_io_context(io_context)
    , m_forward_to_endpoint(asio::ip::address::from_string(forward_to_ip), forward_to_port)
    , m_alive_timeout(std::chrono::seconds(alive_timeout))
    , m_alive_timer(alive_timeout > 0 ? std::make_unique<asio::steady_timer>(m_io_context) : nullptr)
{}

UDPForwardServer::~UDPForwardServer() {}

bool UDPForwardServer::listen(const std::string & eth_ip, uint16_t udp_port)
{
    try
    {
        asio::ip::udp::socket   udp_socket(m_io_context);
        asio::ip::udp::endpoint udp_endpoint;
        if (eth_ip.empty())
            udp_endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), udp_port);
        else
            udp_endpoint = asio::ip::udp::endpoint(asio::ip::make_address(eth_ip), udp_port);

        udp_socket.open(udp_endpoint.protocol());
        // if (m_use_ipv6)
        //     udp_socket.set_option(asio::ip::v6_only(false));

        udp_socket.set_option(asio::ip::udp::socket::reuse_address(true));
        udp_socket.set_option(asio::socket_base::receive_buffer_size(4 * 1024 * 1024));
        udp_socket.set_option(asio::socket_base::send_buffer_size(4 * 1024 * 1024));

        udp_socket.bind(udp_endpoint);

        LOG_INFO("start listen udp {}", fmt::streamed(udp_socket.local_endpoint()));

        m_bind_socket_vec.push_back({ std::move(udp_socket), std::vector<uint8_t>(4 * 1024) });

        return true;
    }
    catch (const std::exception & e)
    {
        LOG_ERROR("listen_port exception:{}", e.what());
        return false;
    }
}

void UDPForwardServer::start()
{
    do_read_data();
    do_check_connection_alive();

    // std::error_code ec;
    // m_io_context.run(ec);
    // LOG_ERROR_IF(ec, "ForwardServer start error:{}", ec.message());
}

void UDPForwardServer::do_read_data()
{
    for (auto & udp_socket : m_bind_socket_vec)
    {
        do_read_data(udp_socket.udp_socket, udp_socket.udp_read_buffer);
    }
}

void UDPForwardServer::do_read_data(asio::ip::udp::socket & udp_socket, std::vector<uint8_t> & udp_read_buffer)
{
    auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
    udp_socket.async_receive_from(
        asio::buffer(udp_read_buffer), *endpoint, [=, &udp_socket, &udp_read_buffer](const std::error_code & ec, std::size_t length) {
            if (ec)
            {
                std::error_code local_endpoint_ec;
                LOG_ERROR_IF(udp_socket.is_open(), "udp socket {} receive from error code:{}, msg:{}", fmt::streamed(udp_socket.local_endpoint(local_endpoint_ec)),
                             ec.value(), ec.message());
                if (udp_socket.is_open())
                {
                    do_read_data(udp_socket, udp_read_buffer);
                }
                return;
            }

            defer_expr_ref(do_read_data(udp_socket, udp_read_buffer));

            std::vector<uint8_t> udp_data;
            udp_data.assign(udp_read_buffer.data(), udp_read_buffer.data() + length);

            auto iter = m_udp_sessions.find(*endpoint);
            if (iter != m_udp_sessions.end())
            {
                auto session = iter->second.lock();
                if (session)
                    session->on_udp_data(std::move(udp_data));
                else
                    m_udp_sessions.erase(iter);
            }
            else
            {
                LOG_INFO("new client address {}, forward to {}", fmt::streamed(*endpoint), fmt::streamed(m_forward_to_endpoint));
                std::shared_ptr<asio::ip::udp::socket> new_udp_socket = std::make_shared<asio::ip::udp::socket>(m_io_context);

                new_udp_socket->async_connect(
                    m_forward_to_endpoint, [=, &udp_socket, udp_data = std::move(udp_data)](const std::error_code & ec) mutable {
                        if (!ec)
                        {
                            auto new_session =
                                std::make_shared<UDPForwardSession>(udp_socket, std::move(*new_udp_socket), *endpoint, m_forward_to_endpoint);
                            new_session->close_handle = [this, endpoint = *endpoint] {
                                m_udp_sessions.erase(endpoint);
                            };
                            m_udp_sessions[*endpoint] = new_session;

                            new_session->on_udp_data(std::move(udp_data));
                            new_session->do_read_data();
                        }
                    });
            }
        });
}

void UDPForwardServer::do_check_connection_alive()
{
    if (m_alive_timer)
    {
        m_alive_timer->expires_from_now(std::chrono::seconds(1));
        m_alive_timer->async_wait([=](const std::error_code & ec) {
            if (ec)
            {
                return;
            }

            auto timeout_time_point = std::chrono::steady_clock::now() - m_alive_timeout;
            for (auto it = m_udp_sessions.begin(); it != m_udp_sessions.end();)
            {
                auto session = it->second.lock();
                if (session && session->last_alive_timepoint() < timeout_time_point)
                {
                    LOG_WARN("{} keepalive timeout, close connection", fmt::streamed(it->first));
                    it = m_udp_sessions.erase(it);
                    session->close();
                }
                else
                    ++it;
            }
            do_check_connection_alive();
        });
    }
}
