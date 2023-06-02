
#include "tun.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/generic/raw_protocol.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/system/system_error.hpp>
#include <cstdio>
#include <cstring>
#include <exception>
#include <linux/if_tun.h>
#include <stdexcept>
#include <system_error>

auto Tun::create(boost::asio::io_context& ctx, std::string_view name, EntryGuard entry_guard, uint netmask_prefix) -> boost::optional<Tun>
{
    ifreq ifr{};
    int fd, err, sockfd;
    sockaddr_in sai{};
	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = htonl(entry_guard.get().to_v4().to_uint());

    strncpy(ifr.ifr_name, name.data(), IFNAMSIZ);
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
		perror("open()");
		return boost::none;
	}

	if((err = ioctl(fd, TUNSETIFF, &ifr)) < 0)
	{
		perror("ioctl(TUNSETIFF)");
		return boost::none;
	}

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		return boost::none;
	}

	// Set MTU
	ifr.ifr_mtu = 1500;
	if((err = ioctl(sockfd, SIOCSIFMTU, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFMTU)");
		return boost::none;
	}

	// Set ipv4 address
	memcpy(&ifr.ifr_addr, &sai, sizeof(sockaddr));
	if((err = ioctl(sockfd, SIOCSIFADDR, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFADDR)");
		return boost::none;
	}

	// Set mask
	sai.sin_addr.s_addr = htonl(static_cast<uint32_t>(-1) << (32 - netmask_prefix)); // Ugly!
	memcpy(&ifr.ifr_netmask, &sai, sizeof(sockaddr));
	if((err = ioctl(sockfd, SIOCSIFNETMASK, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFNETMASK)");
		return boost::none;
	}

	// Get tun flags
	if((err = ioctl(sockfd, SIOCGIFFLAGS, &ifr)) < 0)
	{
		perror("ioctl(SIOCGIFFLAGS)");
		return boost::none;
	}

	// Set tun flags
	ifr.ifr_flags |= IFF_RUNNING | IFF_UP;
	if((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFFLAGS)");
		return boost::none;
	}

    boost::asio::posix::stream_descriptor socket(ctx, fd);
	socket.non_blocking(true);

    return Tun(std::move(socket), ifr.ifr_name, std::move(entry_guard));
}

Tun::Tun(boost::asio::posix::stream_descriptor && socket, std::string_view name, EntryGuard&& entry_guard)
    : m_socket{std::move(socket)}, m_name{name}, m_entry_guard{std::move(entry_guard)}
{

}

auto Tun::read_packet() -> boost::asio::awaitable<boost::optional<Packet>>
{
	char buffer[1500]; // TODO: MTU
	auto tok = as_tuple(boost::asio::use_awaitable);

	auto[ec, read] = co_await m_socket.async_read_some(boost::asio::buffer(&buffer, sizeof(buffer)), tok);
	if(ec)
    {
		std::cerr << ec << std::endl;
        co_return boost::none;
    }

	co_return Packet::from_bytes_unchecked(buffer, read);
}

auto Tun::write_packet(Packet& packet) -> boost::asio::awaitable<bool>
{
	auto tok = as_tuple(boost::asio::use_awaitable);

	auto[ec, read] = co_await m_socket.async_write_some(boost::asio::buffer(packet.data(), packet.size()), tok);
	if(ec)
    {
        co_return false;
    }

	co_return true;
}

auto Tun::name() const -> std::string_view
{
    return m_name;
}

auto Tun::handle_incoming(std::shared_ptr<Tun> stream, DBGuard db_guard) -> boost::asio::awaitable<void>
{
	try{
	while(true) // TODO: shutdown
	{
		auto packet_option = co_await stream->read_packet();
		if(packet_option.has_value() == false)
        {
            std::cerr << "Error in handle_incoming()" << std::endl;
            co_return;
        }

		auto packet = packet_option.value();
		if(packet.check() == false)
		{
			continue;
		}

		auto mq_tx_option = db_guard.get(packet.dst_address().value());
		if(mq_tx_option.has_value() == false)
        {
            continue;
        }

		auto mq_tx = mq_tx_option.get();
        co_await mq_tx.async_send(std::make_shared<Packet>(packet));
	}
	}
	catch(const std::exception& ec)
	{
		std::cerr << ec.what() << std::endl;
	}
}

auto Tun::handle_outgoing(std::shared_ptr<Tun> stream, Receiver mq_rx) -> boost::asio::awaitable<void>
{
	while(true) // TODO shutdown
    {
        auto packet = co_await mq_rx.async_receive();
        auto written_option = co_await stream->write_packet(const_cast<Packet&>(*packet));
        if(written_option == false)
        {
            std::cerr << "Error in handle_outgoing()" << std::endl;
            co_return;
        }
    }
}