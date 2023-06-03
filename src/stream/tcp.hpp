#pragma once

#include "stream/stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

class TCPStream final: public AsyncStream
{
public:
    TCPStream(boost::asio::ip::tcp::socket&&);

    virtual auto write_packet(const Packet&) -> Task<Option<void>> override;
    virtual auto read_packet() -> Task<Option<Packet>> override;

private:
    boost::asio::ip::tcp::socket m_socket;
};