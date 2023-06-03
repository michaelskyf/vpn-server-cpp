#pragma once

#include <boost/utility/result_of.hpp>
#include <common.hpp>
#include "database/entry_guard.hpp"
#include "stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/generic/raw_protocol.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <string_view>
#include <boost/optional.hpp>
#include <database/db_guard.hpp>

class Tun final: AsyncStream
{
public:
    Tun() = delete;

    auto name() const -> std::string_view;

    // Should be buffered
    virtual auto read_exact(char*, size_t) -> Task<Result<void>> override;
    virtual auto write_exact(const char*, size_t) -> Task<Result<void>> override;

private:
    asio::posix::stream_descriptor m_socket;
    std::string m_name;
    EntryGuard m_entry_guard;
};

class TunBuilder
{
public:
    auto name(std::string_view name) -> TunBuilder;
    auto address(asio::ip::network_v4) -> TunBuilder;
    auto address(asio::ip::network_v6) -> TunBuilder;
    auto mtu(uint) -> TunBuilder;
    
    auto build() -> Result<Tun>;
};