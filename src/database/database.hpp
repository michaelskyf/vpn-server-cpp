#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

#include "address_pool.hpp"
#include <mq/mq.hpp>

using ip_mqrx_pair = std::pair<asio::ip::address, Receiver>;

/*! Class storing available ip addresses and registered clients */
class Database
{
friend class DB_RWLock;
public:
	/**
	 * @brief 		Construct the database for ipv4 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	IPv4 network that the database operates on
	 */
    Database(asio::any_io_executor& ctx, asio::ip::network_v4 net);

	/**
	 * @brief 		Construct the database for ipv6 network
	 * @param[in] 	ctx 	Reference to any_io_executor
	 * @param[in] 	net 	Ipv6 network that the database operates on
	 */
    Database(asio::any_io_executor& ctx, asio::ip::network_v6 net);

	/**
	 * @brief 		Register the client in the databse without predefined ip address
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */
    auto register_without_ip() -> Option<ip_mqrx_pair>;

	/**
	 * @brief 		Register the client in the databse with predefined ip address
	 * @param[in] 	address 	Address to be reserved
	 * @returns
	 * 				-success: a pair of reserved address and MQ Receiver
	 *				-failure: boost::None
	 */
    auto register_with_ip(asio::ip::address address) -> Option<ip_mqrx_pair>;

	/**
	 * @brief 		Get MQ Sender from peer
	 * @param[in] 	address
	 * @returns
	 * 				-success: Sender
	 * 				-failure: boost::None
	 */
    auto get(asio::ip::address address) -> Option<Sender>;

private:

	/**
	 * @brief 		Used in EntryGuard to unregister the client from the database
	 */
    auto unregister(asio::ip::address address) -> void;

private:
    asio::any_io_executor& m_ctx;
    AddressPool m_address_pool;
    std::unordered_map<asio::ip::address, Sender> m_map;
};
