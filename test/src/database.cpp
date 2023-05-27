#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio.hpp>
#include <gtest/gtest.h>

#include <db_guard.hpp>

using namespace boost::asio;

TEST(database, simple)
{
    io_context ioContext;

    DBGuard db_guard(ioContext, ip::network_v4(ip::address::from_string("192.168.1.1").to_v4(), 24));

    auto specific_ip = ip::address::from_string("192.168.1.123");
    auto[specific_ip_guard, specific_ip_rx] = db_guard.register_with_ip(specific_ip).value();
    auto [auto_ip_guard, auto_ip_rx] = db_guard.register_without_ip().value();

    EXPECT_EQ(specific_ip_guard.get(), specific_ip);
}