#pragma once

#include "database/entry_guard.hpp"
#include "stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/generic/raw_protocol.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <string_view>
#include <boost/optional.hpp>
#include <database/db_guard.hpp>

class Tun
{
public:
    Tun() = delete;
    static auto create(boost::asio::io_context& ctx, std::string_view name, EntryGuard entry_guard, uint netmask_prefix) -> boost::optional<Tun>;
    static auto handle_incoming(std::shared_ptr<Tun> stream, DBGuard db_guard) -> boost::asio::awaitable<void>;
    static auto handle_outgoing(std::shared_ptr<Tun> stream, Receiver mq_rx) -> boost::asio::awaitable<void>;

    auto read_packet() -> boost::asio::awaitable<boost::optional<Packet>>;
    auto write_packet(Packet&) -> boost::asio::awaitable<bool>;

    auto name() const -> std::string_view;

private:
    Tun(boost::asio::posix::stream_descriptor&&, std::string_view name, EntryGuard&& entry_guard);

private:
    boost::asio::posix::stream_descriptor m_socket;
    std::string m_name;
    EntryGuard m_entry_guard;
};