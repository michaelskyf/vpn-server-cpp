// SPDX-License-Identifier: GPL-3.0-or-later

/* Basic libraries */
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

/* C++ libraries */
#include <vector>
#include <string>

/* Linux libraries */
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>

/* Internal libaries */
#include "server.hpp"

/* OpenSSL libraries */
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

/*
 * main.cpp: Configure the server
 *
 * This part of the program parses command-line arguments and sets-up
 * the server accoring to them
 */

static int create_epfd(int master_epfd, int events = EPOLLIN)
{
	struct epoll_event ev{};
	int epfd = epoll_create1(0);
	if(epfd == -1)
	{
		perror("Failed to create epfd");
		exit(EXIT_FAILURE);
	}

	if(master_epfd == -1)
	{
		return epfd;
	}

	ev.data.fd = epfd;
	ev.events = events;
	if(epoll_ctl(master_epfd, EPOLL_CTL_ADD, epfd, &ev))
	{
		perror("Failed to add file descriptor to epoll");
		exit(EXIT_FAILURE);
	}

	return epfd;
}

static SSL_CTX* create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if(!ctx)
	{
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

/**
 * @brief Initializes data.ctx
 */
static void setup_ssl(struct server_args& data, const char* certfile, const char* keyfile)
{
	data.ctx = create_context();
	if(!data.ctx)
	{
		ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
	}

    /* Set the key and cert */
    if(SSL_CTX_use_certificate_file(data.ctx, certfile, SSL_FILETYPE_PEM) <= 0)
	{
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if(SSL_CTX_use_PrivateKey_file(data.ctx, keyfile, SSL_FILETYPE_PEM) <= 0 )
	{
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

static void print_help(const char* program_name)
{
	printf(
				"Usage: %s [OPTION]...\n"
				"Start VPN server at the given address\n\n"
				""
				"Mandatory arguments to long options are mandatory for short options too.\n"
				"  -a, --address=ADDRESS:PORT\tlisten at given address and port (ipv4 only)\n"
				"  -h, --help\t\t\tshow this message\n"
				"  -i, --interface=NAME\t\tname of the tun interface\n"
				"  -n, --network=NETWORK/MASK\tvirtual network used in the vpn\n"
				"  -p, --certificate=FILENAME\tfile containing a certificate (default is /etc/vpn/cert.pem)\n"
				"  -k, --keyfile=FILENAME\tfile containing a private key (default is /etc/vpn/key.pem)\n"
				"  -c, --config=FILENAME\tfile containing a private key (default is /etc/vpn/key.pem)\n"
				"",
			program_name);
}

static int tun_create(char* name, struct in_addr ip, struct in_addr mask)
{
	struct ifreq ifr{};
	int fd, err, sockfd;
	struct sockaddr_in sai{};
	sai.sin_family = AF_INET;
	sai.sin_addr = ip;

	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
		perror("open()");
		exit(EXIT_FAILURE);
	}

	if((err = ioctl(fd, TUNSETIFF, &ifr)) < 0)
	{
		perror("ioctl(TUNSETIFF)");
		exit(EXIT_FAILURE);
	}

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	// Set MTU
	ifr.ifr_mtu = 1500;
	if((err = ioctl(sockfd, SIOCSIFMTU, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFMTU)");
		exit(EXIT_FAILURE);
	}

	// Set ipv4 address
	memcpy(&ifr.ifr_addr, &sai, sizeof(struct sockaddr));
	if((err = ioctl(sockfd, SIOCSIFADDR, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFADDR)");
		exit(EXIT_FAILURE);
	}

	// Set mask
	sai.sin_addr = mask;
	memcpy(&ifr.ifr_netmask, &sai, sizeof(struct sockaddr));
	if((err = ioctl(sockfd, SIOCSIFNETMASK, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFNETMASK)");
		exit(EXIT_FAILURE);
	}

	// Get tun flags
	if((err = ioctl(sockfd, SIOCGIFFLAGS, &ifr)) < 0)
	{
		perror("ioctl(SIOCGIFFLAGS)");
		exit(EXIT_FAILURE);
	}

	// Set tun flags
	ifr.ifr_flags |= IFF_RUNNING | IFF_UP;
	if((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr)) < 0)
	{
		perror("ioctl(SIOCSIFFLAGS)");
		exit(EXIT_FAILURE);
	}

	strcpy(name, ifr.ifr_name);
	close(sockfd);
	return fd;
}

/**
 * @brief Initializes epfd, epsockfds, epconnfds, epclifds and tunfd in data
 */
static void setup_fds(struct server_args& data, char* tun_name, struct in_addr ip, struct in_addr mask)
{
	data.epfd = create_epfd(-1);
	data.epsockfds = create_epfd(data.epfd);
	data.epconnfds = create_epfd(data.epfd);
	data.epclifds = create_epfd(data.epfd);

	data.tunfd = tun_create(tun_name, ip, mask);
	if(data.tunfd == -1)
	{
		perror("Failed to create tun interface");
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Initializes data.net_addr and data.pool
 */
static void setup_address(struct server_args& data, char* network_string)
{
	uint32_t host_count;
	char* mask_pos = strrchr(network_string, '/');
	if(mask_pos == nullptr)
	{
		fprintf(stderr, "Invalid network address: '%s'\n", network_string);
		exit(EXIT_FAILURE);
	}

	*mask_pos = '\0';
	unsigned int mask_bits = atoi(mask_pos+1);

	host_count = (uint32_t)-1 >> mask_bits;
	data.net_addr = ntohl(inet_addr(network_string));
	if(data.net_addr == (uint32_t)-1 || mask_bits > 32)
	{
		// 255.255.255.255 is not a valid network address
		fprintf(stderr, "Invalid network address: '%s'\n", network_string);
		exit(EXIT_FAILURE);
	}

	data.pool = {host_count};
	try
	{
		data.pool.add(0, {}); // Reseve the first address for network ...
		data.pool.add(1, {}); // ... and the second one for the server
	}
	catch(...)
	{
		fprintf(stderr, "Error during allocation of addresses\n");
		exit(EXIT_FAILURE);
	}
}

static int server_setup(int argc, char* argv[], struct server_args& data)
{
	std::vector<std::string> addresses = {"127.0.0.1:1234"};
	char tun_name[IFNAMSIZ] = "";
	char network_string[sizeof("255.255.255.255/32")] = "10.25.0.0/16";

	int option;
	int option_index = 0;
	int should_parse = 1;

	struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"address", required_argument, 0, 'a'},
		{"interface", required_argument, 0, 'i'},
		{"network", required_argument, 0, 'n'},
		{"certificate", required_argument, 0, 'p'},
		{"keyfile", required_argument, 0, 'k'},
		{"config", required_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	/* Parse args */
	while(should_parse)
	{
		option = getopt_long(argc, argv, "ha:p:i:n:c:k:p:", long_options, &option_index);
		switch(option)
		{
			case 0:
			break;

			case 'h':
				print_help(*argv);
				exit(EXIT_SUCCESS);
			break;

			case 'a':
				addresses.push_back(optarg);
			break;

			case 'i':
				if(strlen(optarg) >= sizeof(tun_name))
				{
					fprintf(stderr, "Tun name '%s' exceeds allowed length\n", optarg);
					exit(EXIT_FAILURE);
				}

				strcpy(tun_name, optarg);
			break;

			case 'n':
				if(strlen(optarg) >= sizeof(network_string))
				{
					fprintf(stderr, "Network address '%s' exceeds allowed length\n", optarg);
					exit(EXIT_FAILURE);
				}

				strcpy(network_string, optarg);
			break;

			case 'p':
			case 'k':
			case 'c':
				printf("todo\n");
			break;

			case '?':
				fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
				exit(EXIT_FAILURE);
			break;

			default:
				should_parse = 0;
			break;
		}
	}

	if(addresses.size() == 0)
	{
		fprintf(stderr, "The server needs at least one address to operate\n");
		return -1;
	}
	
	struct in_addr srv_virt_addr;
	struct in_addr srv_virt_mask;
	setup_address(data, network_string);
	srv_virt_addr.s_addr = htonl(data.net_addr+1);
	srv_virt_mask.s_addr = htonl(~data.pool.size());
	setup_fds(data, tun_name, srv_virt_addr, srv_virt_mask);

	printf("Tun interface '%s' created\n", tun_name);

	for(auto& address : addresses)
	{
		int sockfd;
		int sockopt;
		struct epoll_event ev{};
		struct sockaddr_in addr{};
		size_t found = address.rfind(':');
		if(found == std::string::npos)
		{
			fprintf(stderr, "Invalid address format: '%s'\n", address.c_str());
			return -1;
		}

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd == -1)
		{
			perror("Failed to create a socket");
			return -1;
		}

		addr.sin_addr.s_addr = inet_addr(address.substr(0, found).c_str());
		addr.sin_port = htons(atoi(&address[found+1]));
		addr.sin_family = AF_INET;

		sockopt = 1;
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) == -1)
		{
			perror("Failed to set socket option");
		}

		if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		{
			perror("Failed to bind socket");
			return -1;
		}

		if(listen(sockfd, 32) == -1)
		{
			perror("Failed to listen on a socket");
			return -1;
		}

		ev.data.fd = sockfd;
		ev.events = EPOLLIN;
		if(epoll_ctl(data.epsockfds, EPOLL_CTL_ADD, sockfd, &ev))
		{
			perror("Failed to add file descriptor to epoll");
			return -1;
		}
	}

	struct epoll_event ev{};
	ev.data.fd = data.tunfd;
	ev.events = EPOLLIN;
	if(epoll_ctl(data.epfd, EPOLL_CTL_ADD, data.tunfd, &ev))
	{
		perror("Failed to add file descriptor to epoll");
		return -1;
	}

	
	setup_ssl(data, "/etc/vpn/cert.pem", "/etc/vpn/key.pem");
	return 0;
}

int main(int argc, char* argv[])
{
	struct server_args data;
	int ret;

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	printf("VPN server version %s\n", VERSION);
	if(server_setup(argc, argv, data))
	{
		fprintf(stderr, "Error during server set-up. Exiting\n");
		exit(-1);
	}

	printf("Starting VPN server...\n");
	ret = serve(data);
	
	SSL_CTX_free(data.ctx);
	
	return ret;
}
