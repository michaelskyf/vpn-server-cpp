#include "database/db_guard.hpp"
#include <acceptor/tcp.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/detail/endpoint.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <stream/tun.hpp>

using namespace boost::asio;

int main()
{
    io_context ctx;
    auto bind_address = ip::address::from_string("0.0.0.0");
    ip::tcp::endpoint endpoint(bind_address, 1337);
    ip::tcp::acceptor acceptor(ctx, endpoint);

    DBGuard db_guard(ctx, ip::network_v4(ip::address::from_string("10.10.10.0").to_v4(), 24));

    auto[tun_ip, tun_mq_tx] = db_guard.register_with_ip(ip::address::from_string("10.10.10.1")).value();
    auto tun_option = Tun::create(ctx, "", std::move(tun_ip), 24);
    if(tun_option.has_value() == false)
    {
        std::cerr << "Failed to create TUN device" << std::endl;
        return -1;
    }

    auto tun = std::move(tun_option.value());
    auto stream = std::make_shared<Tun>(std::move(tun));
    co_spawn(ctx, Tun::handle_incoming(stream, db_guard), boost::asio::detached);
    co_spawn(ctx, Tun::handle_outgoing(stream, tun_mq_tx), boost::asio::detached);

    TCPAcceptor tcp_acceptor(ctx, db_guard, std::move(acceptor));

    co_spawn(ctx, tcp_acceptor.accept(), detached);

    ctx.run();
}