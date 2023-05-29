#include "tcp.hpp"

TCPStream::TCPStream(boost::asio::ip::tcp::socket&& socket)
    : m_socket{std::move(socket)}
{

}

auto TCPStream::read_exact(char* data, size_t size) -> boost::asio::awaitable<bool>
{
    size_t read_bytes = 0;
    auto tok = as_tuple(boost::asio::use_awaitable);
    
    while(read_bytes != size)
    {
        auto[ec, tmp] = co_await m_socket.async_receive(boost::asio::buffer(data, size), tok);
        if(ec)
        {
            co_return false;
        }
        read_bytes += tmp;
    }
    
    co_return true;
}

auto TCPStream::write_exact(char* data, size_t size) -> boost::asio::awaitable<bool>
{
    size_t written_bytes = 0;
    auto tok = as_tuple(boost::asio::use_awaitable);
    
    while(written_bytes != size)
    {
        auto[ec, tmp] = co_await m_socket.async_send(boost::asio::buffer(data, size), tok);
        if(ec)
        {
            co_return false;
        }
        written_bytes += tmp;
    }

    co_return true;
}