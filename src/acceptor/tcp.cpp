#include "tcp.hpp"
#include "connection/connection.hpp"
#include "database/db_guard.hpp"
#include "stream/tcp.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <memory>

TCPAcceptor::TCPAcceptor(asio::any_io_executor& ctx, DBGuard db_guard, boost::asio::ip::tcp::acceptor&& acceptor)
    : m_ctx{ctx}, m_db_guard{db_guard}, m_acceptor{std::move(acceptor)}
{

}

auto TCPAcceptor::accept() -> boost::asio::awaitable<void>
{
    while(true) // TODO: shutdown
    {
        auto client = co_await m_acceptor.async_accept(boost::asio::use_awaitable);

		client.set_option(boost::asio::socket_base::keep_alive(true));

        auto stream = std::make_shared<TCPStream>(std::move(client));

        boost::asio::co_spawn(m_ctx, handle_client(m_ctx, stream, m_db_guard), boost::asio::detached);
    }
}