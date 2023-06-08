#pragma once

#include <boost/asio.hpp>
#include <common.hpp>

/*! Interface abstracting reading and writing from various protocols */
class AsyncStream
{
public:

    /**
     * @brief Read exact amount of bytes from the stream
     * @param[in] buf   Pointer to a buffer
     * @param[in] size  Size of the buffer
     * @returns
     *          - void on success
     *          - error on failure
     */
    virtual auto read_exact(char* buf, size_t size) -> Task<Result<void>> = 0;
    
    /**
     * @brief Write exact amount of bytes to the stream
     * @param[in] data   Pointer to the data
     * @param[in] size  Size of the data
     * @returns
     *          - void on success
     *          - error on failure
     */
    virtual auto write_exact(const char* data, size_t size) -> Task<Result<void>> = 0;
    virtual ~AsyncStream() = default;
};