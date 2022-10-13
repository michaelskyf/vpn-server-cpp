// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>

/* OpenSSL libraries */
#include <openssl/ssl.h>
#include <openssl/err.h>

/**
 * @brief Wrapper for epoll_ctl()
 *
 * @param[in]	epfd				epoll file descriptor
 * @param[in]	fd					file descriptor to be added to epfd
 * @param[in]	events				events that epoll should wait for
 *
 * @returns 0 on success, -1 on error
 */
static int epoll_add(int epfd, int fd, size_t index, int events)
{
	struct epoll_event ev{};

	ev.data.u64 = index;
	ev.events = events;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev))
	{
		perror("Failed to add file descriptor to epoll");
		return -1;
	}

	return 0;
}

/**
 * @brief Read exactly buffer_length amount of data
 *
 * @param[in]	out_fd				output file descriptor
 * @param[in]	hdr					ipv4 header
 */
 __attribute__((unused))
static void ipv4_hdr_dump(int out_fd, struct ip* hdr)
{
	dprintf(out_fd,
	"Version: %d\n"
	"Header length: %d\n"
	"Total length: %d\n"
	"Identification: %d\n"
	"Offset: %d\n"
	"Time To Live: %d\n"
	"Protocol: %d\n"
	"Checksum: %d\n"
	"Source: %s\n"
	"Destination: %s\n"
	"",
	hdr->ip_v,
	hdr->ip_hl,
	ntohs(hdr->ip_len),
	hdr->ip_id,
	hdr->ip_off,
	hdr->ip_ttl,
	hdr->ip_p,
	hdr->ip_sum,
	inet_ntoa(hdr->ip_src),
	inet_ntoa(hdr->ip_dst)
	);
}

// TODO: Remove this function. It may cause problems with malicious/broken clients
static int ssl_write_n(SSL* ssl, const char* buffer, size_t length)
{
	while(length)
	{
		printf("write_n\n");
		size_t written;
		int ret;
		if((ret = SSL_write_ex(ssl, buffer, length, &written)) <= 0)
		{
			if(SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE)
				continue;

			ERR_print_errors_fp(stderr);
			return -1;
		}

		buffer += written;
		length -= written;
	}

	return 0;
}

// TODO: Remove this function. It may cause problems with malicious/broken clients
static int ssl_read_n(SSL* ssl, char* buffer, size_t length)
{
	while(length)
	{
		printf("read_n\n");
		size_t written;
		if((written = SSL_read(ssl, buffer, length)) <= 0)
		{
			if(SSL_get_error(ssl, written) == SSL_ERROR_WANT_READ)
				continue;

			ERR_print_errors_fp(stderr);
			return -1;
		}

		buffer += written;
		length -= written;
	}

	return 0;
}