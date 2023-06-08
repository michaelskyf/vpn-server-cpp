#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include "database.hpp"
#include <common.h>
#include <mq/mq.hpp>

using ip_mqrx_pair = std::pair<boost::asio::ip::address, Receiver>;

/*! Guard allowing multiple readers and single writer */
class DB_RWLock
{
friend class EntryGuard;
public:
	/**
	 * @brief 		Construct the database for ipv4 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	IPv4 network that the database operates on
	 */
    DB_RWLock(asio::any_io_executor& ctx, asio::ip::network_v4 net)
        : m_rwlock{}, m_database(ctx, net)
    {

    }

	/**
	 * @brief 		Construct the database for ipv6 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	IPv6 network that the database operates on
	 */
    DB_RWLock(asio::any_io_executor& ctx, asio::ip::network_v6 net)
        : m_rwlock{}, m_database(ctx, net)
    {

    }

	/**
	 * @brief 		Register the client in the databse without predefined ip address
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */
    auto register_without_ip() -> Option<ip_mqrx_pair>
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.register_without_ip();
    }

	/**
	 * @brief 		Register the client in the databse with predefined ip address
	 * @param[in] 	address 	Address to be reserved
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */
    auto register_with_ip(boost::asio::ip::address address) -> boost::optional<ip_mqrx_pair>
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.register_with_ip(address);
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
        boost::shared_lock<boost::shared_mutex> lock(m_rwlock);

        return m_database.get(address);
    }

private:

	/**
	 * @brief 		Used in EntryGuard to unregister the client from the database
	 */
    auto unregister(boost::asio::ip::address address) -> void
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_rwlock);

        m_database.unregister(address);
    }

private:
    boost::shared_mutex m_rwlock;
    Database m_database;
};
