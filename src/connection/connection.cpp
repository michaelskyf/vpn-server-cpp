#include "connection.hpp"
#include "database/entry_guard.hpp"
#include <mq/mq.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

struct __attribute__((packed)) login_info
{
	char username[64];
	char password[256];
};

struct __attribute__((packed)) handshake_reply
{
	uint8_t status; // 0-ACK, 255-NACK
	uint32_t address;
	uint32_t mask;
	uint16_t mtu;
};

namespace
{
    auto handle_incoming(AsyncStream& stream, EntryGuard& ip_guard, DBGuard& db_guard) -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet_option = co_await Packet::from_stream(stream);
            if(packet_option.has_value() == false)
            {
                std::cerr << "Error in handle_incoming()" << std::endl;
                co_return;
            }

            auto packet = packet_option.value();

            auto mq_tx_option = db_guard.get(packet.dst_address());
            if(mq_tx_option.has_value() == false)
            {
                continue;
            }

            auto mq_tx = mq_tx_option.get();
            co_await mq_tx.async_send(packet);
        }
    }

    auto handle_outgoing(AsyncStream& stream, EntryGuard& ip_guard, Receiver& mq_rx) -> boost::asio::awaitable<void>
    {
        while(true) // TODO shutdown
        {
            auto packet = co_await mq_rx.async_receive();
            auto written_option = co_await stream.write_exact(packet.data(), packet.size());
            if(written_option == false)
            {
                std::cerr << "Error in handle_outgoing()" << std::endl;
                co_return;
            }
        }
    }

    // 1. Receive login and password
    // 2. Send handshake_reply
    auto handle_handshake(AsyncStream& stream, DBGuard& db_guard) -> boost::asio::awaitable<boost::optional<std::pair<EntryGuard, Receiver>>>
    {
        login_info credentials;

        co_await stream.read_exact(reinterpret_cast<char*>(&credentials), sizeof(credentials));

        printf("login: %.*s\npassword: %.*s\n",
                    static_cast<int>(sizeof(credentials.username)),
                    credentials.username,
                    static_cast<int>(sizeof(credentials.password)),
                    credentials.password);

        auto register_option = db_guard.register_without_ip();  
        if(register_option.has_value() == false)
        {
            co_return boost::none;
        }

        auto[ip_guard, mq_rx] = std::move(register_option.get());
        handshake_reply reply{};

        reply.mtu = 1500;
        reply.status = 0;
        reply.mask = 0xFFFFFF00; // TODO
        reply.address = ip_guard.get().to_v4().to_uint();

        auto reply_option = co_await stream.write_exact(reinterpret_cast<char*>(&reply), sizeof(reply));
        if(reply_option == false)
        {
            co_return boost::none;
        }

        co_return std::pair<EntryGuard, Receiver>(std::move(ip_guard), mq_rx);
    }
}

/// Initialize the connection and run in/out handlers
auto handle_client(boost::asio::io_context& ctx, std::unique_ptr<AsyncStream> stream, DBGuard db_guard) -> boost::asio::awaitable<void>
{
    auto handshake_option = co_await handle_handshake(*stream, db_guard);
    if(handshake_option.has_value() == false)
    {
        co_return;
    }

    auto[ip_guard, mq_rx] = std::move(handshake_option.get());

    auto frx = boost::asio::co_spawn(ctx, handle_incoming(*stream, ip_guard, db_guard), boost::asio::use_future);
    auto ftx = boost::asio::co_spawn(ctx, handle_outgoing(*stream, ip_guard, mq_rx), boost::asio::use_future);

    frx.wait();
    ftx.wait();
}