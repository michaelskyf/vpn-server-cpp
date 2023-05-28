#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/optional.hpp>
#include <vector>
#include "stream.hpp"
#include <linux/ip.h>

class Packet
{
public:
    /// Invalid packet! Never use this constructor. (Used to please MQ) (Or use check())
    Packet() = default;

    static auto from_bytes(char* array, size_t size) -> Packet;
    static auto from_bytes_unchecked(char* array, size_t size) -> Packet
    {
        return Packet(array, size);
    }

    static auto from_stream(Stream& stream) -> boost::asio::awaitable<boost::optional<Packet>>;

    auto data() -> char*
    {
        return m_data.data();
    }

    auto size() -> size_t
    {
        return m_data.size();
    }

    auto src_address() -> boost::asio::ip::address;
    auto dst_address() -> boost::asio::ip::address;
    auto check() -> bool;

private:    
    Packet(char* array, size_t size)
        : m_data{std::vector<char>(array, array+size)}
    {

    }

private:
    std::vector<char> m_data;
};