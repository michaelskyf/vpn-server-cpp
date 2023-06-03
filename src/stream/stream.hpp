#pragma once

#include <boost/asio.hpp>
#include <packet/packet.hpp>
#include <common.hpp>

class AsyncStream
{
public:
    virtual auto read_packet() -> Task<Option<Packet>> = 0;
    virtual auto write_packet(const Packet&) -> Task<Option<void>> = 0;
    virtual ~AsyncStream() = default;
};