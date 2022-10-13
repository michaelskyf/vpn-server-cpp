// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <unistd.h>
#include "address_pool.hpp"
#include <vector>
#include <memory>

/* OpenSSL libraries */
#include <openssl/ossl_typ.h>
#include <openssl/ssl.h>

struct ssl_deleter
{
	void operator() (SSL* ssl)
	{
		if(ssl)
		{
			SSL_shutdown(ssl);
			close(SSL_get_fd(ssl));
			SSL_free(ssl);
		}
	}
};

class net_buffer
{
public:
	net_buffer(size_t size = 0)
	{
		resize(size);
	}

	ssize_t fill(SSL* ssl) noexcept
	{
		move_data();
		ssize_t read_bytes = SSL_read(ssl, data()+used_m, data_m.size() - used_m);
		if(read_bytes <= 0)
			return read_bytes;

		used_m += read_bytes;

		return read_bytes;
	}

	ssize_t fill(void* src, size_t amount) noexcept
	{
		if(amount > left())
			amount = left();

		move_data();
		memcpy(data()+used_m, src, amount);

		used_m += amount;

		return amount;
	}

	ssize_t write(SSL* ssl, size_t amount = -1) noexcept
	{
		if(amount > used_m)
			amount = used_m;

		ssize_t written = SSL_write(ssl, data()+offset_m, amount);
		if(written <= 0)
			return written;

		used_m -= written;
		offset_m += written;

		return written;
	}

	const char* data() const noexcept
	{
		return data_m.data();
	}

	char* data() noexcept
	{
		return data_m.data();
	}

	const char* unread_data() const noexcept
	{
		return data_m.data()+offset_m;
	}

	char* unread_data() noexcept
	{
		return data_m.data()+offset_m;
	}

	size_t used() const noexcept
	{
		return used_m;
	}

	size_t size() const noexcept
	{
		return data_m.size();
	}

	size_t left() const noexcept
	{
		return data_m.size() - used_m;
	}

	size_t offset() const noexcept
	{
		return offset_m;
	}

	void move_data()
	{
		if(offset_m)
		{
			memmove(data(), data()+offset_m, used_m);
			offset_m = 0;
		}
	}

	void resize(size_t new_size)
	{
		data_m.resize(new_size);
	}

	void mark_as_read(size_t amount)
	{
		if(amount > used_m)
			amount = used_m;

		offset_m += amount;
		used_m -= amount;
	}

private:
	/// Offset to unread data
	size_t offset_m = 0;
	/// How much buffer is currently used
	size_t used_m = 0;
	/// Data
	std::vector<char> data_m;
};

struct client_data
{
	client_data() = default;
	~client_data() = default;

	client_data(SSL* ssl_ptr, size_t bufsiz): sndbuf{bufsiz}, rcvbuf{bufsiz}, ssl_m{ssl_ptr}
	{

	}

	client_data& operator= (client_data&& other)
	{
		sndbuf = std::move(other.sndbuf);
		rcvbuf = std::move(other.rcvbuf);
		ssl_m = std::move(other.ssl_m);

		return *this;
	}

	net_buffer sndbuf;
	net_buffer rcvbuf;
	std::unique_ptr<SSL, ssl_deleter> ssl_m;
	friend bool operator==(const client_data&, const client_data&);
};

struct server_args
{
	int epsockfds;
	int epconnfds;
	int epclifds;
	int tunfd;
	int epfd;
	uint32_t net_addr;
	SSL_CTX* ctx;
	address_pool<client_data> pool;
};

struct __attribute__((packed)) login_info
{
	char username[64];
	char password[256];
};

struct __attribute__((packed)) handshake_reply
{
	uint8_t status; // 0-ACK, 255-NACK
	uint32_t address;
	uint32_t mask;
	uint16_t mtu;
};

/**
 * @brief Main function of the server
 *
 * @param[in]	args			server_args structure filled with data
 *
 * @returns						0 on success, non-zero on failure
 */
int serve(struct server_args& args);
