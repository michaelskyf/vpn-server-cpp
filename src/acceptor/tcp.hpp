#pragma once

#include "database/db_guard.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <stream/tcp.hpp>

class TCPAcceptor final
{
public:
    TCPAcceptor(boost::asio::io_context& ctx, DBGuard db_guard, boost::asio::ip::tcp::acceptor&& acceptor);

    /**
     *
     */
    auto accept() -> boost::asio::awaitable<void>;

private:
    boost::asio::io_context& m_ctx;
    DBGuard m_db_guard;
    boost::asio::ip::tcp::acceptor m_acceptor;
};