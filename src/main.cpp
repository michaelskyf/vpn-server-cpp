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

using namespace boost::asio;

int main()
{
    io_context ctx;
    auto bind_address = ip::address::from_string("0.0.0.0");
    ip::tcp::endpoint endpoint(bind_address, 1337);
    ip::tcp::acceptor acceptor(ctx, endpoint);

    DBGuard db_guard(ctx, ip::network_v4(ip::address::from_string("10.10.10.0").to_v4(), 24));

    TCPAcceptor tcp_acceptor(ctx, db_guard, std::move(acceptor));

    co_spawn(ctx, tcp_acceptor.accept(), detached);

    ctx.run();
}