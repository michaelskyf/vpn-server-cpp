#pragma once

#include <common.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/optional.hpp>
#include <vector>

class Packet
{
public:
    Packet(const Packet&) = delete;
    Packet(Packet&&);

    static auto from_bytes(char* array, size_t size) -> Packet;
    static auto from_bytes_unchecked(char* array, size_t size) -> Packet;

    auto data() const -> const char*;
    auto size() const -> size_t;

    auto src_address() const -> asio::ip::address;
    auto dst_address() const -> asio::ip::address;
    auto check() const -> bool;

private:    
    Packet(char* array, size_t size);

private:
    std::vector<char> m_data;
};