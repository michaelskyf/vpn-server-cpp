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
    static auto create(asio::io_context& ctx, std::string_view name, EntryGuard entry_guard, uint netmask_prefix) -> Option<Tun>;

    auto name() const -> std::string_view;

    virtual auto write_packet(const Packet&) -> Task<Option<void>> override;
    virtual auto read_packet() -> Task<Option<Packet>> override;

private:
    Tun(asio::posix::stream_descriptor&&, std::string_view name, EntryGuard&& entry_guard);

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
    auto mtu(uint) -> TunBuilder;
    
    auto build() -> Option<Tun>;
};