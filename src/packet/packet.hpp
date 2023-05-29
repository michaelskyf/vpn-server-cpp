#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/optional.hpp>
#include <vector>
#include "stream/stream.hpp"

class Packet
{
public:
    /// Invalid packet! Never use this constructor. (Used to please MQ) (Or use check())
    Packet() = default;

    static auto from_bytes(char* array, size_t size) -> Packet;
    static auto from_bytes_unchecked(char* array, size_t size) -> Packet;

    static auto from_stream(AsyncStream& stream) -> boost::asio::awaitable<boost::optional<Packet>>;

    auto data() -> char*;
    auto size() -> size_t;

    auto src_address() -> boost::optional<boost::asio::ip::address>;
    auto dst_address() -> boost::optional<boost::asio::ip::address>;
    auto check() -> bool;

private:    
    Packet(char* array, size_t size);

private:
    std::vector<char> m_data;
};