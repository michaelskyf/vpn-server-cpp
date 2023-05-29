#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include "database.hpp"
#include <mq/mq.hpp>

using ip_mqrx_pair = std::pair<boost::asio::ip::address, Receiver>;

class DB_RWLock
{
friend class EntryGuard;
public:
    DB_RWLock(boost::asio::io_context& ctx, boost::asio::ip::network_v4 net)
        : m_rwlock{}, m_database(ctx, net)
    {

    }

    auto register_without_ip() -> boost::optional<ip_mqrx_pair>
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.register_without_ip();
    }

    auto register_with_ip(boost::asio::ip::address address) -> boost::optional<ip_mqrx_pair>
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.register_with_ip(address);
    }

    auto get(boost::asio::ip::address address) -> boost::optional<Sender>
    {
        boost::shared_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.get(address);
    }

private:
    auto unregister(boost::asio::ip::address address) -> void
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        m_database.unregister(address);
    }

private:
    boost::shared_mutex m_rwlock;
    Database m_database;
};