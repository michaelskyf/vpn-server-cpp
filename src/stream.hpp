#pragma once

#include <boost/asio.hpp>

class Stream
{
public:
    virtual auto read(char*, size_t) -> boost::asio::awaitable<std::optional<size_t>> = 0;
    virtual auto write(char*, size_t) -> boost::asio::awaitable<std::optional<size_t>> = 0;
    virtual ~Stream() = default;
};