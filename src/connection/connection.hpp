#pragma once

#include "database/entry_guard.hpp"
#include "db_guard.hpp"
#include "mq.hpp"
#include "packet.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/thread/future.hpp>
#include <memory>
#include <optional>

// TODO: move definitions to .cpp (and remove inline)

namespace
{
    inline auto handle_incoming(std::shared_ptr<Stream> stream, std::shared_ptr<EntryGuard> ip_guard, DBGuard db_guard) -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet_option = co_await Packet::from_stream(*stream);
            if(packet_option.has_value() == false)
            {
                std::cerr << "Error in handle_incoming()" << std::endl;
                co_return;
            }

            auto packet = packet_option.value();

            auto mq_tx_option = db_guard.get(packet.dst_address());
            if(mq_tx_option.has_value() == false)
            {
                continue;
            }

            auto mq_tx = mq_tx_option.get();
            co_await mq_tx.async_send(packet);
        }
    }

    inline auto handle_outgoing(std::shared_ptr<Stream> stream, std::shared_ptr<EntryGuard> ip_guard, Receiver mq_rx) -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet = co_await mq_rx.async_receive();
            auto written_option = co_await stream->write(packet.data(), packet.size());
            if(written_option.has_value() == false)
            {
                std::cerr << "Error in handle_outgoing()" << std::endl;
                co_return;
            }
        }
    }
}

inline auto handle_client(boost::asio::io_context& ctx, std::shared_ptr<Stream>&& stream, DBGuard db_guard) -> bool
{
    auto register_option = db_guard.register_without_ip();
    if(register_option.has_value() == false)
    {
        return false;
    }

    auto[ip_guard, mq_rx] = std::move(register_option.get());
    auto ip_guard_ptr = std::make_shared<EntryGuard>(std::move(ip_guard));

    boost::asio::co_spawn(ctx, handle_incoming(stream, ip_guard_ptr, db_guard), boost::asio::detached);
    boost::asio::co_spawn(ctx, handle_outgoing(stream, ip_guard_ptr, mq_rx), boost::asio::detached);

    return true;
}