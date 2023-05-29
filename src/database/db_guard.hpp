#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional.hpp>
#include <memory>
#include <boost/optional.hpp>

#include "database/database.hpp"
#include "database/db_rwlock.hpp"
#include <mq/mq.hpp>
#include "entry_guard.hpp"

namespace ip = boost::asio::ip;

// Used for asynchronous access to database containing ip's and mq's
class DBGuard
{
public:
    DBGuard(boost::asio::io_context& ctx, ip::network_v4 net)
        : m_database_rwlock{std::make_shared<DB_RWLock>(ctx, net)}
    {

    }

    auto register_without_ip() -> boost::optional<std::pair<EntryGuard, Receiver>>
    {
        auto option = m_database_rwlock->register_without_ip();
        if(option.has_value() == false)
        {
            return boost::none;
        }

        return {{EntryGuard(m_database_rwlock, option.get().first), option.get().second}};
    }

    auto register_with_ip(ip::address address) -> boost::optional<std::pair<EntryGuard, Receiver>>
    {
        auto option = m_database_rwlock->register_with_ip(address);
        if(option.has_value() == false)
        {
            return boost::none;
        }

        return {{EntryGuard(m_database_rwlock, option.get().first), option.get().second}};
    }

    auto get(ip::address address) -> boost::optional<Sender>
    {
        return m_database_rwlock->get(address);
    }

private:
    std::shared_ptr<DB_RWLock> m_database_rwlock;
};