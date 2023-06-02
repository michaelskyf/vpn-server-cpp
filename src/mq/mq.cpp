#include "mq.hpp"

using namespace boost;

Sender::Sender(std::shared_ptr<Subject>& c)
    : m_channel{c}
{}

Receiver::Receiver(std::shared_ptr<Subject>& c)
    : m_channel{c}
{}

auto Sender::async_send(SharedPacket packet) -> asio::awaitable<void>
{
    return m_channel->async_notify_all(packet);
}

auto Receiver::async_receive() -> asio::awaitable<SharedPacket>
{
    auto observer = std::make_shared<Channel>(co_await asio::this_coro::executor);
    m_channel->add_observer(observer);

    std::shared_ptr<Packet const> msg;

    try {
        msg = co_await observer->async_receive();
    } catch (boost::system::system_error const& se) {
        trace(" error: ", se.code().message());
        co_return nullptr;
    }

    observer->close(); // pretty redundant, destructor already makes sure

    co_return msg;
}