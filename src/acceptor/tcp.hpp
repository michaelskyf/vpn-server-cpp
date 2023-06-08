#pragma once

#include "database/db_guard.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <stream/tcp.hpp>

/*! Acceptor for TCP connections */
class TCPAcceptor final
{
public:
    TCPAcceptor(asio::any_io_executor& ctx, DBGuard db_guard, asio::ip::tcp::acceptor&& acceptor);

	/**
	 * @brief Waits for a new connection and calls ConnectionHandler
	 */
    auto accept() -> Task<void>;

private:
    asio::any_io_executor& m_ctx;
    DBGuard m_db_guard;
    asio::ip::tcp::acceptor m_acceptor;
};
