#include "packet.hpp"

//auto Packet::from_bytes(char* array, size_t size) -> Packet;

auto Packet::from_bytes_unchecked(char* array, size_t size) -> Packet
{
    return Packet(array, size);
}

//auto Packet::from_stream(AsyncStream& stream) -> boost::asio::awaitable<boost::optional<Packet>>;

auto Packet::data() -> char*
{
    return m_data.data();
}

auto Packet::size() -> size_t
{
    return m_data.size();
}

//auto Packet::src_address() -> boost::asio::ip::address;

//auto Packet::dst_address() -> boost::asio::ip::address;

//auto Packet::check() -> bool;

Packet::Packet(char* array, size_t size)
    : m_data{std::vector<char>(array, array+size)}
{

}