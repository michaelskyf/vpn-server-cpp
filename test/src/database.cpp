#include <gtest/gtest.h>

#include <database/database.hpp>
#include <database/db_guard.hpp>

using namespace boost::asio;

TEST(database, simple)
{
    io_context ioContext;

    DBGuard db_guard(ioContext, ip::network_v4(ip::address::from_string("192.168.1.1").to_v4(), 24));

    auto specific_ip = ip::address::from_string("192.168.1.123");
    auto[specific_ip_guard, specific_ip_rx] = db_guard.register_with_ip(specific_ip).value();
    auto [auto_ip_guard, auto_ip_rx] = db_guard.register_without_ip().value();

    EXPECT_EQ(specific_ip_guard.get(), specific_ip);
    EXPECT_NE(specific_ip_guard.get(), auto_ip_guard.get());
}

namespace database_mq
{
    auto receive(Receiver rx, std::vector<char>& result) -> boost::asio::awaitable<void>
    {
        auto received = co_await rx.async_receive();
        result = std::vector<char>(received.data(), received.data()+received.size());
    }

    auto send(Sender tx, std::vector<char>& payload) -> boost::asio::awaitable<void>
    {
        co_await tx.async_send(Packet::from_bytes_unchecked(payload.data(), payload.size()));
    }
}

TEST(database, mq)
{
    io_context ioContext;

    DBGuard db_guard(ioContext, ip::network_v4(ip::address::from_string("192.168.1.1").to_v4(), 24));

    auto specific_ip = ip::address::from_string("192.168.1.123");
    auto[specific_ip_guard, specific_ip_rx] = db_guard.register_with_ip(specific_ip).value();
    auto [auto_ip_guard, auto_ip_rx] = db_guard.register_without_ip().value();
    
    auto specific_ip_tx = db_guard.get(specific_ip).value();

    std::vector<char> result;
    std::vector<char> payload = {'H', 'E', 'L', 'L', 'O', '\0'};
    boost::asio::co_spawn(ioContext, database_mq::receive(specific_ip_rx, result), detached);
    boost::asio::co_spawn(ioContext, database_mq::send(specific_ip_tx, payload), detached);

    ioContext.run();

    EXPECT_EQ(result, payload);
}