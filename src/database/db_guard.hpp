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

/*! Wrapper for std::shared_ptr<DB_RWLock> */
class DBGuard
{
public:
	/**
	 * @brief 		Construct the database for ipv4 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	IPv4 network that the database operates on
	 */
    DBGuard(boost::asio::io_context& ctx, boost::asio::ip::network_v4 net)
        : m_database_rwlock{std::make_shared<DB_RWLock>(ctx, net)}
    {

    }

	/**
	 * @brief 		Construct the database for ipv6 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	IPv6 network that the database operates on
	 */
    DBGuard(boost::asio::io_context& ctx, boost::asio::ip::network_v6 net)
        : m_database_rwlock{std::make_shared<DB_RWLock>(ctx, net)}
    {

    }

	/**
	 * @brief 		Register the client in the databse without predefined ip address
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */

    auto register_without_ip() -> boost::optional<std::pair<EntryGuard, Receiver>>
    {
        auto option = m_database_rwlock->register_without_ip();
        if(option.has_value() == false)
        {
            return boost::none;
        }

        return {{EntryGuard(m_database_rwlock, option.get().first), option.get().second}};
    }

	/**
	 * @brief 		Register the client in the databse with predefined ip address
	 * @param[in] 	address 	Address to be reserved
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */
    auto register_with_ip(boost::asio::ip::address address) -> boost::optional<std::pair<EntryGuard, Receiver>>
    {
        auto option = m_database_rwlock->register_with_ip(address);
        if(option.has_value() == false)
        {
            return boost::none;
        }

        return {{EntryGuard(m_database_rwlock, option.get().first), option.get().second}};
    }

	/**
	 * @brief 		Get MQ Sender from peer
	 * @param[in] 	address
	 * @returns
	 * 				-success: Sender
	 * 				-failure: boost::None
	 */
    auto get(boost::asio::ip::address address) -> boost::optional<Sender>
    {
        return m_database_rwlock->get(address);
    }

private:
    std::shared_ptr<DB_RWLock> m_database_rwlock;
};
