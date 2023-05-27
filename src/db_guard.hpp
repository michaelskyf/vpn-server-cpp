#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/optional/optional.hpp>
#include <memory>
#include <boost/optional.hpp>

#include "database/database.hpp"
#include "mq.hpp"

namespace ip = boost::asio::ip;

class EntryGuard
{
public:
    

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Used for asynchronous access to database containing ip's and mq's
class db_guard
{
public:
    db_guard(ip::network_v4 net);
    auto register_without_ip() -> boost::optional<EntryGuard>;
    auto register_with_ip(ip::address_v4) -> boost::optional<EntryGuard>;
    auto get(ip::address_v4) -> boost::optional<Sender>;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};