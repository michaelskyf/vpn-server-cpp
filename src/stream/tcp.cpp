#include "tcp.hpp"
#include <iostream>

TCPStream::TCPStream(boost::asio::ip::tcp::socket&& socket)
    : m_socket{std::move(socket)}
{

}

auto TCPStream::read_exact(char* data, size_t size) -> boost::asio::awaitable<bool>
{
    auto tok = as_tuple(boost::asio::use_awaitable);
    
    while(size != 0)
    {
        auto[ec, tmp] = co_await m_socket.async_receive(boost::asio::buffer(data, size), tok);
        if(ec)
        {
            std::cerr << ec << std::endl;
            co_return false;
        }
        size -= tmp;
        data += tmp;
    }
    
    co_return true;
}

auto TCPStream::write_exact(char* data, size_t size) -> boost::asio::awaitable<bool>
{
    auto tok = as_tuple(boost::asio::use_awaitable);
    
    while(size != 0)
    {
        auto[ec, tmp] = co_await m_socket.async_send(boost::asio::buffer(data, size), tok);
        if(ec)
        {
            std::cerr << ec << std::endl;
            co_return false;
        }

        size -= tmp;
        data += tmp;
    }

    co_return true;
}