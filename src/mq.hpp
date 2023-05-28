#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/optional/optional.hpp>
#include <memory>

#include "packet.hpp"

using Channel = boost::asio::experimental::concurrent_channel<void(
        boost::system::error_code, Packet)>;

class Sender
{
public:
    Sender(std::shared_ptr<Channel>&);

    auto async_send(Packet packet) -> boost::asio::awaitable<void>;

private:
    std::shared_ptr<Channel> m_channel;
};

class Receiver
{
public:
    Receiver(std::shared_ptr<Channel>&);

    auto async_receive() -> boost::asio::awaitable<Packet>;

private:
    std::shared_ptr<Channel> m_channel;
};