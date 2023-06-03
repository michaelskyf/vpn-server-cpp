#pragma once

#include <stream/stream.hpp>
#include <common.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <vector>

class Packet
{
public:
    Packet(const Packet&) = delete;
    Packet(Packet&&);

    static auto from_stream(AsyncStream& stream) -> Task<Result<Packet>>;
    static auto from_bytes(char* array, size_t size) -> Result<Packet>;
    static auto from_bytes_unchecked(char* array, size_t size) -> Packet;

    auto data() const -> const char*;
    auto len() const -> size_t;

    auto src() const -> asio::ip::address;
    auto dst() const -> asio::ip::address;
    auto version() const -> uint;

private:    
    Packet(char* array, size_t size);

private:
    std::vector<char> m_data;
};