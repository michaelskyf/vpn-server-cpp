#pragma once
#include <common.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/optional/optional.hpp>
#include <memory>

#include <packet/packet.hpp>

using SharedPacket = std::shared_ptr<Packet const>;
using Channel = asio::use_awaitable_t<asio::any_io_executor>::as_default_on_t<asio::experimental::concurrent_channel<void(
        boost::system::error_code, SharedPacket)>>;

/*! Used for sending packets over mpmc channel */
class Sender
{
public:
    Sender(std::shared_ptr<Channel>&);

	/**
	 * @brief Sends packet to the channel
	 */
    auto async_send(SharedPacket packet) -> Task<void>;

private:
    std::shared_ptr<Channel> m_channel;
};

/*! Used for receiving packets from mpmc channel */
class Receiver
{
public:
    Receiver(std::shared_ptr<Channel>&);

	/**
	 * @brief Receives packet from the channel
	 */
    auto async_receive() -> Task<SharedPacket>;

private:
    std::shared_ptr<Channel> m_channel;
};
