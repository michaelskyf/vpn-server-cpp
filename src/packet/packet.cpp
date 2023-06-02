#include "packet.hpp"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/none.hpp>
#include <netinet/ip.h>
#include <stdexcept>

//auto Packet::from_bytes(char* array, size_t size) -> Packet;

auto Packet::from_bytes_unchecked(char* array, size_t size) -> Packet
{
    return Packet(array, size);
}

auto Packet::from_stream(AsyncStream& stream) -> boost::asio::awaitable<boost::optional<Packet>>
{
    char packet_buffer[1500]; // TODO: Variable MTU
    ip* hdr = reinterpret_cast<ip*>(&packet_buffer);

    if(co_await stream.read_exact(packet_buffer, sizeof(ip)) == false) // Read the header
    {
        co_return boost::none;
    }

    if(hdr->ip_v != 4) // TODO: ipv6
    {
        co_return boost::none;
    }
    
    if(ntohs(hdr->ip_len) > 1500 || ntohs(hdr->ip_len) < sizeof(ip))
    {
        co_return boost::none; // TODO: MTU
    }

    if(co_await stream.read_exact(packet_buffer + sizeof(ip), ntohs(hdr->ip_len) - sizeof(ip)) == false) // Read the rest of the packet
    {
        co_return boost::none;
    }

    co_return Packet::from_bytes_unchecked(packet_buffer, ntohs(hdr->ip_len));
}

auto Packet::data() const -> const char*
{
    return m_data.data();
}

auto Packet::size() const -> size_t
{
    return m_data.size();
}

auto Packet::src_address() const -> boost::optional<boost::asio::ip::address>
{
    if(check() == true)
    {
        return boost::asio::ip::address(boost::asio::ip::address_v4(ntohl(reinterpret_cast<const ip*>(data())->ip_src.s_addr)));
    }

    return boost::none;
}

auto Packet::dst_address() const -> boost::optional<boost::asio::ip::address>
{
    if(check() == true)
    {
        return boost::asio::ip::address(boost::asio::ip::address_v4(ntohl(reinterpret_cast<const ip*>(data())->ip_dst.s_addr)));
    }

    return boost::none;
}

auto Packet::check() const -> bool
{
    if(m_data.size() < sizeof(ip))
    {
        return false;
    }

    auto* packet = reinterpret_cast<const ip*>(data());

    // TODO: allow ipv6
    if(packet->ip_v != 4)
    {
        return false;
    }

    if(ntohs(packet->ip_len) != size())
    {
        return false;
    }

    return true;
}

Packet::Packet(char* array, size_t size)
    : m_data{std::vector<char>(array, array+size)}
{

}