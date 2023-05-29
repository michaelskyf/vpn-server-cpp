#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

#include "address_pool.hpp"
#include <mq/mq.hpp>

using ip_mqrx_pair = std::pair<boost::asio::ip::address, Receiver>;

class Database
{
friend class DB_RWLock;
public:
    Database(boost::asio::io_context& ctx, boost::asio::ip::network_v4 net);
    auto register_without_ip() -> boost::optional<ip_mqrx_pair>;
    auto register_with_ip(boost::asio::ip::address) -> boost::optional<ip_mqrx_pair>;
    auto get(boost::asio::ip::address) -> boost::optional<Sender>;

private:
    auto unregister(boost::asio::ip::address) -> void;

private:
    boost::asio::io_context& m_ctx;
    AddressPool m_address_pool;
    std::unordered_map<boost::asio::ip::address, Sender> m_map;
};