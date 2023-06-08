#pragma once

#include <stream/stream.hpp>
#include <common.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <vector>

/*! Abstraction over raw bytes forming a packet */
class Packet
{
public:
	/**
	 * @brief Call from_bytes(), from_stream() or from_bytes_unchecked()
	 */
    Packet(const Packet&) = delete;
    Packet(Packet&&);

	/**
	 * @brief 			Reads bytes from AsyncStream and if they form a valid packet, returns it
	 * @param[in] 		stream 		AsyncStream from which the packet will get read
	 * @returns
	 *					-success: Packet
	 *					-failure: Error
	 */
    static auto from_stream(AsyncStream& stream) -> Task<Result<Packet>>;

	/**
	 * @brief 			Checks if the vector forms a valid packet and if it does, returns the object
	 * @param[in] 		data 		vector with the packet
	 * @returns
	 *					-success: Packet
	 *					-failure: Error
	 */
    static auto from_vector(std::vector<char>&& data) -> Result<Packet>;

	/**
	 * @brief 			Forms a packet from the given vector (Unsafe)
	 * @param[in] 		data 		vector with the packet
	 * @returns
	 *					-success: Packet
	 *					-failure: Error
	 */
    static auto from_vector_unchecked(std::vector<char>&& data) -> Packet;

	/**
	 * @returns 		Raw bytes forming a packet
	 */
    auto data() const -> const char*;

	/**
	 * @returns 		Length of the packet
	 */
    auto len() const -> size_t;

	/**
	 * @returns 		Packet's source address
	 */
    auto src() const -> asio::ip::address;

	/**
	 * @returns 		Packet's destination address
	 */
    auto dst() const -> asio::ip::address;

	/**
	 * @returns 		Packet's version
	 */
    auto version() const -> uint;

private:
    Packet(char* array, size_t size);

private:
    std::vector<char> m_data;
};
