#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <openssl/ssl.h>
#include <unistd.h>

#include <net/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <wait.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../src/server.hpp"

// TODO: Remove this function. It may cause problems with malicious/broken clients
static int ssl_write_n(SSL* ssl, const char* buffer, size_t length)
{
	while(length)
	{
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

int tunnel_create(char* name)
{
	struct ifreq ifr;
	int fd, err;

	if((fd = open("/dev/net/tun", O_RDWR)) < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ);

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if((err = ioctl(fd, TUNSETIFF, &ifr)) < 0)
	{
		close(fd);
		return err;
	}

	return fd;
}

void tunnel_setup()
{
	system("/sbin/ip link set tun0 up");
	
}

int serve(int epfd, int tunfd, SSL* ssl)
{
	char buffer[1500];
	ssize_t read_bytes;
	struct epoll_event events[32];

	while(true)
	{
		int nfds = epoll_wait(epfd, events, sizeof(events)/sizeof(*events), -1);
		if(nfds == -1)
		{
			perror("epoll_wait()");
			return -1;
		}

		for(int i = 0; i < nfds; i++)
		{
			struct epoll_event& event = events[i];

			if(event.data.fd == tunfd)
			{
				struct ip* hdr = (struct ip*)buffer;
				read_bytes = read(tunfd, buffer, sizeof(buffer));
				if(read_bytes == -1)
				{
					perror("tunnel read()");
					return -1;
				}

				if((size_t)read_bytes < sizeof(*hdr))
				{
					printf("read\n");
				}

				if(hdr->ip_v != 4)
				{
					continue;
				}

				if(ntohs(hdr->ip_len) != read_bytes)
				{
					printf("len\n");
				}

				if(ssl_write_n(ssl, buffer, read_bytes))
				{
					fprintf(stderr, "send_n()\n");
					continue;
				}
			}
			else
			{
				while(1)
				{
				unsigned int to_read;
				struct ip* hdr = (struct ip*)buffer;
				read_bytes = SSL_peek(ssl, buffer, sizeof(*hdr));
				if(read_bytes <= 0)
				{
					if(SSL_get_error(ssl, read_bytes) == SSL_ERROR_WANT_WRITE)
						break;

					ERR_print_errors_fp(stderr);
					break;
				}
				else if(read_bytes != sizeof(*hdr))
				{
					break;
				}

				to_read = ntohs(hdr->ip_len);

				if(to_read > sizeof(buffer))
				{
					printf("ohno to_read: %u %d\n", to_read, hdr->ip_v);
					break;
				}

				if(ssl_read_n(ssl, buffer, to_read))
				{
					fprintf(stderr, "ssl_read_n\n");
					break;
				}

				if(write(tunfd, buffer, to_read) == -1)
				{
					if(errno == EINVAL)
					{
						printf("invalid\n");
						break;
					}

					perror("write()");
					break;
				}
				}
			}
		}
	}
}

SSL_CTX* InitCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLS_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

int main()
{

	// Create a tunnel
	char tunnel_name[IFNAMSIZ] = "tun0";
	struct epoll_event ev;
	int tunfd, epfd;
	SSL_CTX *ctx;
	SSL* ssl;

	

	ev.events = EPOLLIN;
	epfd = epoll_create1(EPOLL_CLOEXEC);
	if(epfd == -1)
	{
		perror("epoll_create1()");
		exit(-1);
	}

	tunfd = tunnel_create(tunnel_name);
	if(tunfd < 0)
	{
		perror("create_tunnel()");
		return -1;
	}

	tunnel_setup();

	SSL_library_init();
	ctx = InitCTX();
	ssl = SSL_new(ctx);
	
	// Ignore errors since this is a test client

	// Connect to server...
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = inet_addr("10.152.128.1");
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		perror("socket()");
	if(connect(sockfd, (sockaddr*)&addr, addr_len) != 0)
		perror("connect()");

	SSL_set_fd(ssl, sockfd);
	if ( SSL_connect(ssl) <= 0 )
        ERR_print_errors_fp(stderr);

	ev.events = EPOLLIN;
	ev.data.fd = tunfd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, tunfd, &ev) == -1)
	{
		perror("epoll_ctl()");
		exit(-1);
	}

	ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP;
	ev.data.ptr = (void*)ssl;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
	{
		perror("epoll_ctl()");
		exit(-1);
	}

	char buffer[sizeof(struct login_info)];
	struct login_info* info = (struct login_info*)buffer;
	struct handshake_reply* reply = (struct handshake_reply*)buffer;

	strncpy(info->username, "test-user", sizeof(info->username));
	strncpy(info->password, "VeryStr0ngP@55W0Rd", sizeof(info->password));

	if(ssl_write_n(ssl, buffer, sizeof(*info)))
	{
		printf("send_n failed\n");
		return -1;
	}

	if(ssl_read_n(ssl, buffer, sizeof(*reply)))
	{
		printf("read_n failed\n");
		return -1;
	}

	if(reply->status != 0)
	{
		printf("status not 0\n");
		return -1;
	}

	uint32_t mask = ntohl(reply->mask);
	int bits = 0;
	for(int i = 0; i < 32 && mask != 0; i++)
	{
		mask = mask << 1;
		bits++;
	}

	if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1)
		{
			perror("fcntl()");
			close(sockfd);
			return -1;
		}
	
	struct in_addr server_addr{};
	struct in_addr client_addr{};
	server_addr = inet_makeaddr(ntohl(reply->address & reply->mask), 1);
	client_addr.s_addr = reply->address;

	// tun0 should be replaced with the name we get, but the client is for testing only
	system(("/sbin/ip address add " + std::string(inet_ntoa(client_addr)) + '/' + std::to_string(bits) + " dev tun0").c_str());
	system(("/sbin/ip route add default via " + std::string(inet_ntoa(server_addr)) + " dev tun0").c_str());

	printf("%s\n", ("/sbin/ip address add " + std::string(inet_ntoa(client_addr)) + '/' + std::to_string(bits) + " dev tun0").c_str());
	printf("%s\n", ("/sbin/ip route add default via " + std::string(inet_ntoa(server_addr)) + " dev tun0").c_str());

	return serve(epfd, tunfd, ssl);
}
