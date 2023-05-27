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

namespace simple
{
    auto tx_runner(Sender snd) -> boost::asio::awaitable<void>
    {
        auto payload = 'X';
        co_await snd.async_send(payload);
        std::cout << "Sent payload: " << payload << std::endl;
    }

    auto rx_runner(Receiver rcv) -> boost::asio::awaitable<void>
    {
        auto char_ptr = co_await rcv.async_receive();

        std::cout << "Received payload: " << *char_ptr << std::endl;
    }
}

TEST(mq, simple)
{
    boost::asio::io_context ioContext;
    Channel c = Channel(boost::asio::make_strand(ioContext), 2);
    auto c_ptr = std::make_shared<Channel>(std::move(c));

    Sender tx(c_ptr);
    Receiver rx(c_ptr);
    
    boost::asio::co_spawn(ioContext, simple::rx_runner(rx), boost::asio::detached);
    boost::asio::co_spawn(ioContext, simple::tx_runner(tx), boost::asio::detached);

    ioContext.run();
}