// SPDX-License-Identifier: GPL-3.0-or-later

/* Basic libraries */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* C++ libraries */
#include <stdexcept>

/* Linux libraries */
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

/* Internal libaries */
#include "server.hpp"
#include "util.hpp"

/* OpenSSL libraries */
#include <openssl/ssl.h>
#include <openssl/err.h>

bool operator==(const client_data& a, const client_data& b)
{
	return a.ssl_m == b.ssl_m;
}

/**
 * @brief Removes given client from lookup_table
 *
 * @param[in]	client			client_data reference
 * @param[in]	lookup_table	address pool
 */
static void remove_client(client_data& client, address_pool<client_data>& lookup_table)
{
	try
	{
		size_t index = lookup_table.find(client);
		lookup_table.del(index);
	}
	catch(...)
	{
		return;
	}
}

/**
 * @brief Forward packets received from tun interface to client
 *
 * @param[in]	tunfd			tun file descriptor
 * @param[in]	lookup_table	address pool
 *
 * @returns 0 on success, -1 on error, 1 to signal that the client has been removed
 */
static int forward_to_client(int tunfd, address_pool<client_data>& lookup_table)
{
	char buffer[1500];
	struct ip* hdr = (struct ip*)buffer;
	ssize_t read_bytes;
	client_data* client;

	read_bytes = read(tunfd, buffer, sizeof(buffer));
	if(read_bytes == -1)
	{
		perror("tun read()");
		return -1;
	}

	if((size_t)read_bytes < sizeof(*hdr))
	{
		return -1;
	}

	if(hdr->ip_v != 4)
	{
		return 0;
	}

	try
	{
		client = &lookup_table.get(ntohl(hdr->ip_dst.s_addr) & (uint32_t)lookup_table.size());
	}
	catch(...)
	{
		return 0;
	}

	if(client->sndbuf.left() >= (size_t)read_bytes)
	{
		client->sndbuf.fill(hdr, read_bytes);
	}

	ssize_t written = client->sndbuf.write(client->ssl_m.get());
	if(written <= 0)
	{
		if(SSL_get_error(client->ssl_m.get(), written) == SSL_ERROR_WANT_WRITE)
		{
			return 0;
		}
		
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
}

/**
 * @brief Forward packets received from client to tun interface
 *
 * @param[in]	client			client_data reference
 * @param[in]	tunfd			tun file descriptor
 *
 * @returns 0 on success, -1 on error, 1 to signal that the client should be removed
 */
static int forward_to_tun(client_data& client, int tunfd)
{
	auto& buff = client.rcvbuf;

	while(true)
	{
		ssize_t read_bytes = buff.fill(client.ssl_m.get());
		if(read_bytes <= 0)
		{
			if(SSL_get_error(client.ssl_m.get(), read_bytes) == SSL_ERROR_WANT_READ)
			{
				return 0;
			}

			ERR_print_errors_fp(stderr);
			return -1;
		}
	
		while(buff.used() >= sizeof(struct ip))
		{
			size_t to_read;
			ssize_t tmp;
			struct ip* hdr = (struct ip*)buff.unread_data();
			
			to_read = ntohs(hdr->ip_len);

			if(to_read > buff.size())
			{
				return 1;
			}

			if(to_read > buff.used())
			{
				break;
			}

			tmp = write(tunfd, hdr, to_read);
			if(tmp <= 0)
			{
				if(errno == EINVAL)
				{
					fprintf(stderr, "Client sent invalid packet\n");
					return 1;
				}

				perror("write()");
				return -1;
			}

			buff.mark_as_read(tmp);
		}
	}
}

/**
 * @brief Authenticate clients and add them to epconnfds
 *
 * @param[in]	epsockfd		epoll file descriptor with listening sockets
 * @param[in]	epconnfds		epoll file descriptor with new clients
 * @param[in]	ctx				pointer to SSL_CTX structure
 *
 * @returns -1 on error, otherwise number of clients added to epfd
 */
static int accept_new_clients(int epsockfds, int epconnfds, SSL_CTX* ctx)
{
	struct epoll_event events[64];
	int nfds;
	int sockopt;
	int accepted_clients = 0;
	SSL* ssl;

	nfds = epoll_wait(epsockfds, events, sizeof(events)/sizeof(*events), -1);
	if(nfds == -1 && errno != EINTR)
	{
		perror("epoll_wait()");
		return -1;
	}

	for(int i = 0; i < nfds; i++)
	{
		int fd = events[i].data.fd;
		struct sockaddr_storage addr;
		socklen_t addr_len = sizeof(addr);

		int clientfd = accept(fd, (struct sockaddr*)&addr, &addr_len);
		if(clientfd == -1)
		{
			perror("accept()");
			return -1;
		}

		sockopt = 1;
		if(setsockopt(clientfd, SOL_SOCKET, SO_KEEPALIVE, &sockopt, sizeof(sockopt)))
		{
			perror("setsockopt()");
			return -1;
		}

		sockopt = 10;
		if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPIDLE, &sockopt, sizeof(sockopt)))
		{
			perror("setsockopt()");
			return -1;
		}

		sockopt = 5;
		if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPINTVL, &sockopt, sizeof(sockopt)))
		{
			perror("setsockopt()");
			return -1;
		}

		sockopt = 5;
		if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPCNT, &sockopt, sizeof(sockopt)))
		{
			perror("setsockopt()");
			return -1;
		}

		ssl = SSL_new(ctx);
		if(!ssl)
		{
			fprintf(stderr, "Failed to create a new ssl\n");
			close(clientfd);
			return -1;
		}

        if(SSL_set_fd(ssl, clientfd) <= 0)
		{
			ERR_print_errors_fp(stderr);
			close(clientfd);
			SSL_free(ssl);
			return -1;
		}

		if(SSL_accept(ssl) <= 0)
		{
			// TODO check errors
			ERR_print_errors_fp(stderr);
			close(clientfd);
			SSL_free(ssl);
			return -1;
		}

		struct epoll_event ev{};
		ev.data.ptr = ssl;
		ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP;
		if(epoll_ctl(epconnfds, EPOLL_CTL_ADD, clientfd, &ev))
		{
			perror("epoll_ctl()");
			close(clientfd);
			SSL_free(ssl);
			return -1;
		}
		
		if(fcntl(clientfd, F_SETFL, fcntl(clientfd, F_GETFL, 0) | O_NONBLOCK) == -1)
		{
			perror("fcntl()");
			close(clientfd);
			return -1;
		}

		accepted_clients++;
	}

	return accepted_clients;
}

/**
 * @brief Authenticate clients and add them to epfd
 *
 * @param[in]	epfd			main epoll file descriptor
 * @param[in]	epconnfds		epoll file descriptor with new clients
 * @param[in]	lookup_table	address pool
 * @param[in]	net_addr		address of the virtual network
 *
 * @returns -1 on error, otherwise number of clients added to epfd
 */
static int handshake_new_clients(int epfd, int epconnfds, address_pool<client_data>& lookup_table, int net_addr)
{
	struct epoll_event events[64];
	int nfds;
	int accepted_clients = 0;
	char buffer[std::max(sizeof(struct handshake_reply), sizeof(struct login_info))];

	nfds = epoll_wait(epconnfds, events, sizeof(events)/sizeof(*events), -1);
	if(nfds == -1 && errno != EINTR)
	{
		perror("epoll_wait()");
		return -1;
	}

	for(int i = 0; i < nfds; i++)
	{
		SSL* ssl = (SSL*)events[i].data.ptr;
		int fd = SSL_get_fd(ssl);
		int host;
		struct login_info* info = (struct login_info*)buffer;
		struct handshake_reply* reply = (struct handshake_reply*)buffer;
		client_data* client;

		try
		{
			lookup_table.add((host = lookup_table.get_free_index()), {ssl, 1500});
			client = &lookup_table.get(host);
		}
		catch(const std::runtime_error& error)
		{
			fprintf(stderr, "Failed to insert a new connection to lookup_table: %s\n", error.what());
			SSL_shutdown(ssl);
			close(SSL_get_fd(ssl));
			SSL_free(ssl);
			return -1;
		}

		if(ssl_read_n(ssl, buffer, sizeof(*info))) // TODO: ssl_read_n can block indefinitely
		{
			remove_client(*client, lookup_table);
			continue;
		}

		printf("login: %.*s\npassword: %.*s\n", (int)sizeof(info->username), info->username, (int)sizeof(info->password), info->password);
		printf("todo: check if credentials are correct\n");

		if(epoll_ctl(epconnfds, EPOLL_CTL_DEL, fd, NULL) == -1)
		{
			perror("epoll_ctl()");
			remove_client(*client, lookup_table);
			return -1;
		}

		if(epoll_add(epfd, fd, host, EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP) == -1)
		{
			remove_client(*client, lookup_table);
			return -1;
		}

		reply->status = 0;
		reply->address = inet_makeaddr(net_addr, host).s_addr;
		reply->mask = ~htonl(lookup_table.size());
		reply->mtu = htons(1500);
		if(ssl_write_n(ssl, (char*)reply, sizeof(*reply)))
		{
			remove_client(*client, lookup_table);
			continue;
		}

		accepted_clients++;
	}

	return accepted_clients;
}


static int client_tun_loop(int epfd, int tunfd, address_pool<client_data>& lookup_table)
{
	struct epoll_event events[64];
	int nfds;

	nfds = epoll_wait(epfd, events, sizeof(events)/sizeof(*events), -1);
	if(nfds == -1 && errno != EINTR)
	{
		perror("epoll_wait()");
		return -1;
	}

	for(int i = 0; i < nfds; i++)
	{
		int event = events[i].events;
		client_data* client;

		try
		{
			client = &lookup_table.get(events[i].data.u64);
		}
		catch(...)
		{
			return -1;
		}

		int ret = forward_to_tun(*client, tunfd);
		if(ret || event & (EPOLLHUP | EPOLLRDHUP))
		{
			remove_client(*client, lookup_table);
			printf("Removed a client\n");
		}
	}

	return 0;
}

int serve(struct server_args& args)
{
	auto& lookup_table = args.pool;
	struct epoll_event events[64];
	int nfds;

	while(true)
	{
		nfds = epoll_wait(args.epfd, events, sizeof(events)/sizeof(*events), -1);
		if(nfds == -1 && errno != EINTR)
		{
			perror("epoll_wait()");
			return -1;
		}

		for(int i = 0; i < nfds; i++)
		{
			int event = events[i].events;
			int fd = events[i].data.fd;

			if(fd == args.tunfd)
			{
				// Lookup the client and send the packet to it
				
				if(event & (EPOLLHUP | EPOLLRDHUP))
				{
					fprintf(stderr, "tun closed\n");
					return -1;
				}

				int ret = forward_to_client(fd, lookup_table);
				if(ret == 1)
					fprintf(stderr, "Removed client due to internal error\n");
				else if(ret == -1)
					fprintf(stderr, "Error in forward_to_client()\n");
			}
			else if(fd == args.epsockfds)
			{
				// Accept new client and add it to epconnfds

				int ret = accept_new_clients(args.epsockfds, args.epconnfds, args.ctx);
				if(ret != -1)
					printf("Accepted %d client(s)!\n", ret);
				else
					fprintf(stderr, "Error while accepting new clients\n");
			}
			else if(fd == args.epconnfds)
			{
				// Receive login credentials and promote new clients if they are correct

				int ret = handshake_new_clients(args.epclifds, args.epconnfds, lookup_table, args.net_addr);
				if(ret != -1)
					printf("Authenticated %d client(s)!\n", ret);
				else
					fprintf(stderr, "Error while authenticating clients\n");
					
			}
			else if(fd == args.epclifds)
			{
				// Forward the packet from client to tun
				
				client_tun_loop(args.epclifds, args.tunfd, lookup_table);
			}
		}
	}

	return 0;
}
