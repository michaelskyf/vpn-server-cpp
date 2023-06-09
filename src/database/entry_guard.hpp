#pragma once

#include <boost/asio/ip/address.hpp>
#include <memory>
#include "db_rwlock.hpp"

/*! Database client entry guard - on destruction calls Database::unregister() */
class EntryGuard
{
public:
    EntryGuard(std::shared_ptr<DB_RWLock> db_rwlock, boost::asio::ip::address address)
        : m_database_rwlock{db_rwlock}, m_address{address}
    {

    }

    EntryGuard(const EntryGuard&) = delete;
    EntryGuard(EntryGuard&& other) noexcept
        : m_database_rwlock{other.m_database_rwlock}, m_address{other.m_address}, m_destroyed{other.m_destroyed}
    {
        other.m_destroyed = true;
    }

    auto get() -> boost::asio::ip::address
    {
        return m_address;
    }

    ~EntryGuard()
    {
        if(m_destroyed == false)
        {
            m_database_rwlock->unregister(m_address);
        }
    }

private:
    std::shared_ptr<DB_RWLock> m_database_rwlock;
    boost::asio::ip::address m_address;
    bool m_destroyed = false;
};
