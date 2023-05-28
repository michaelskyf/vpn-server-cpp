#include "packet.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/strand.hpp>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <mq.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <vector>

namespace simple
{
    auto tx_runner(Sender snd, Packet payload, size_t id) -> boost::asio::awaitable<void>
    {
        co_await snd.async_send(payload);
    }

    auto rx_runner(Receiver rcv, size_t id) -> boost::asio::awaitable<void>
    {
        auto char_ptr = co_await rcv.async_receive();
    }
}

TEST(mq, simple)
{
    boost::asio::io_context ioContext;
    Channel c = Channel(ioContext, 0);
    auto c_ptr = std::make_shared<Channel>(std::move(c));

    Sender tx(c_ptr);
    Receiver rx(c_ptr);
    
    boost::asio::co_spawn(ioContext, simple::rx_runner(rx, 0), boost::asio::detached);
    boost::asio::co_spawn(ioContext, simple::tx_runner(tx, {'H', 'E', 'L', 'L', 'O'}, 1), boost::asio::detached);

    ioContext.run();
}