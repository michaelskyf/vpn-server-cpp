#pragma once

#include <boost/asio.hpp>

class AsyncStream
{
public:
    virtual auto read_exact(char*, size_t) -> boost::asio::awaitable<bool> = 0;
    virtual auto write_exact(char*, size_t) -> boost::asio::awaitable<bool> = 0;
    virtual ~AsyncStream() = default;
};