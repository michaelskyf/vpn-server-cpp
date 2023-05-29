#pragma once

#include "stream/stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

class TCPStream final: public AsyncStream
{
public:
    TCPStream(boost::asio::ip::tcp::socket&&);

    virtual auto read_exact(char* data, size_t size) -> boost::asio::awaitable<bool> override;

    virtual auto write_exact(char* data, size_t size) -> boost::asio::awaitable<bool> override;

private:
    boost::asio::ip::tcp::socket m_socket;
};