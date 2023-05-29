#include "mq.hpp"

using namespace boost;

Sender::Sender(std::shared_ptr<Channel>& c)
    : m_channel{c}
{}

Receiver::Receiver(std::shared_ptr<Channel>& c)
    : m_channel{c}
{}

auto Sender::async_send(Packet packet) -> asio::awaitable<void>
{
    return m_channel->async_send(system::error_code{}, packet, asio::use_awaitable);
}

auto Receiver::async_receive() -> asio::awaitable<Packet>
{
    return m_channel->async_receive(asio::use_awaitable);
}