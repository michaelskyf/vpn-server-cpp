#pragma once

#include <boost/asio.hpp>
#include <common.hpp>

class AsyncStream
{
public:
    virtual auto read_exact(char*, size_t) -> Task<Result<void>> = 0;
    virtual auto write_exact(const char*, size_t) -> Task<Result<void>> = 0;
    virtual ~AsyncStream() = default;
};