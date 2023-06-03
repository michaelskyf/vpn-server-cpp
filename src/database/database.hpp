#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

#include "address_pool.hpp"
#include <mq/mq.hpp>

using ip_mqrx_pair = std::pair<asio::ip::address, Receiver>;

class Database
{
friend class DB_RWLock;
public:
    Database(asio::any_io_executor& ctx, asio::ip::network_v4 net);
    Database(asio::any_io_executor& ctx, asio::ip::network_v6 net);

    auto register_without_ip() -> Option<ip_mqrx_pair>;
    auto register_with_ip(asio::ip::address) -> Option<ip_mqrx_pair>;
    auto get(asio::ip::address) -> Option<Sender>;

private:
    auto unregister(asio::ip::address) -> void;

private:
    asio::any_io_executor& m_ctx;
    AddressPool m_address_pool;
    std::unordered_map<asio::ip::address, Sender> m_map;
};