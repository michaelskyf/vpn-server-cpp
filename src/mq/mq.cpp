#include "mq.hpp"

using namespace boost;

Sender::Sender(std::shared_ptr<Channel>& c)
    : m_channel{c}
{}

Receiver::Receiver(std::shared_ptr<Channel>& c)
    : m_channel{c}
{}

auto Sender::async_send(SharedPacket packet) -> Task<void>
{
    return m_channel->async_send({}, packet);
}

auto Receiver::async_receive() -> Task<SharedPacket>
{
    return m_channel->async_receive();
}