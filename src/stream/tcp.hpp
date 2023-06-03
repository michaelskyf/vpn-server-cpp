#pragma once

#include "stream/stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

class TCPStream final: public AsyncStream
{
public:
    TCPStream(boost::asio::ip::tcp::socket&&);

    virtual auto read_exact(char*, size_t) -> Task<Result<void>> override;
    virtual auto write_exact(const char*, size_t) -> Task<Result<void>> override;

private:
    boost::asio::ip::tcp::socket m_socket;
};