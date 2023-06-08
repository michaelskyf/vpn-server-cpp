#pragma once

#include <boost/utility/result_of.hpp>
#include <common.hpp>
#include "database/entry_guard.hpp"
#include "stream.hpp"
#include <boost/asio.hpp>
#include <boost/asio/generic/raw_protocol.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <string_view>
#include <boost/optional.hpp>
#include <database/db_guard.hpp>

/*! Implementation of AsyncStream interface for Linux' TUN interface */
class Tun final: AsyncStream
{
public:
	/**
	 * @brief Use class TunBuilder for constructing the class
	*/
    Tun() = delete;

	/**
	 * @brief Returns name of the interface
	 * @return Name of the interface
	 */
    auto name() const -> std::string_view;

    // TODO: IMPL INFO: (read/write)_exact need to be buffered, because Tun returns a whole packet on read, but Packet::from_stream calls read_exact a few times
	// to ensure that the packet is correctly received
    virtual auto read_exact(char*, size_t) -> Task<Result<void>> override;
    virtual auto write_exact(const char*, size_t) -> Task<Result<void>> override;

private:
    asio::posix::stream_descriptor m_socket;
    std::string m_name;
    EntryGuard m_entry_guard;
};

/*! Used for constructing the TUN class */
class TunBuilder
{
public:
	/**
	 * @brief Set the name of the interface
	 */
    auto name(std::string_view name) -> TunBuilder;

	/**
	 * @brief Add the ipv4 address to the interface
	 */
    auto address(asio::ip::network_v4) -> TunBuilder;

	/**
	 * @brief Add the ipv6 address to the interface
	 */
    auto address(asio::ip::network_v6) -> TunBuilder;

	/**
	 * @brief Set the MTU of the interface
	 */
    auto mtu(uint) -> TunBuilder;

	/**
	 * @brief Build the interface
	 * @returns
	 * 			-Tun on success
	 * 			-Error on failure
	 */
    auto build() -> Result<Tun>;
};
