#pragma once

#include "db_guard.hpp"
#include "mq.hpp"
#include "packet.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/use_future.hpp>
#include <optional>

class Handler
{
public:
    Handler(boost::asio::io_context& ctx, std::unique_ptr<Stream>&& stream, DBGuard db_guard)
        : m_ctx{ctx}, m_stream{std::move(stream)}, m_db_guard(db_guard)
    {
        
    }

    auto handle_incoming() -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet_option = co_await Packet::from_stream(*m_stream);
            if(packet_option.has_value() == false)
            {
                std::cerr << "Error in handle_incoming()" << std::endl;
                co_return;
            }

            auto packet = packet_option.value();

            auto mq_tx_option = m_db_guard.get(packet.dst_address());
            if(mq_tx_option.has_value() == false)
            {
                continue;
            }

            auto mq_tx = mq_tx_option.get();
            co_await mq_tx.async_send(packet);
        }
    }

    auto handle_outgoing(Receiver mq_rx) -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet = co_await mq_rx.async_receive();
            auto written_option = co_await m_stream->write(packet.data(), packet.size());
            if(written_option.has_value() == false)
            {
                std::cerr << "Error in handle_outgoing()" << std::endl;
                co_return;
            }
        }
    }

    auto run() -> void
    {
        auto register_option = m_db_guard.register_without_ip();
        if(register_option.has_value() == false)
        {
            return;
        }

        auto[ip_guard, mq_rx] = std::move(register_option.get());

        auto future_rx = boost::asio::co_spawn(m_ctx, handle_incoming(), boost::asio::use_future);
    }

private:
    boost::asio::io_context& m_ctx;
    std::unique_ptr<Stream> m_stream;
    DBGuard m_db_guard;
};