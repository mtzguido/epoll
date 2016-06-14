#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common.h"

void usage(char *name) {
	fprintf(stderr, "uso: %s <ip> <puerto>\n", name);
	exit(1);
}

static void print_event(int fd, struct epoll_event ev) {
	char flags_str[200];

	flags_str[0] = 0;
	flags_str[1] = 0; /* dirty */

	if (ev.events & EPOLLIN )	strcat(flags_str, "|EPOLLIN");
	if (ev.events & EPOLLOUT)	strcat(flags_str, "|EPOLLOUT");
	if (ev.events & EPOLLERR)	strcat(flags_str, "|EPOLLERR");
	if (ev.events & EPOLLHUP)	strcat(flags_str, "|EPOLLHUP");
	if (ev.events & EPOLLRDHUP)	strcat(flags_str, "|EPOLLRDHUP");
	if (ev.events & EPOLLPRI)	strcat(flags_str, "|EPOLLPRI");
	if (ev.events & EPOLLET)	strcat(flags_str, "|EPOLLET");
	if (ev.events & EPOLLONESHOT)	strcat(flags_str, "|EPOLLONESHOT");

	fprintf(stderr, "Event for fd %i. Flags=(%s).\n",
			fd, flags_str+1);
}


int create_sock(char *host, char *port);
int setup_epoll(int sockfd);

int main(int argc, char **argv) {
	int sock, epfd;
	struct epoll_event ev;
	char buf[100];
	int n, count;

	if (argc != 3)
		usage(argv[0]);

	sock = create_sock(argv[1], argv[2]);
	epfd = setup_epoll(sock);

	count = 2;
	while (count > 0) {
		fprintf(stderr, "count=%i\n", count);

		n = epoll_wait(epfd, &ev, 1, -1);
		if (n < 0)
			die("epoll_wait");

		/*
		 * n debe ser igual a 1, si o si, ya que
		 * no hubo error ni timeout
		 */
		assert(n == 1);

		int fd = ev.data.fd;
		print_event(fd, ev);

		if (ev.events & EPOLLERR)
			die("epollerr");

		if (fd == 0) {
			int t;
			t = read(0, buf, 100);

			if (t == 0) {
				/*
				 * Cuando cerramos todas las refencias
				 * a un archivo, se remueve automaticamente
				 * del epoll. Pero tenemos mas referencias
				 * a la tty!
				 */
				epoll_ctl(epfd, EPOLL_CTL_DEL, 0, NULL);
				close(0);

				/*
				 * De paso, cerramos el extremo escritor
				 * del socket
				 */
				shutdown(sock, SHUT_WR);
				count--;
			} else {
				write(sock, buf, t);
			}
		} else if (fd == sock) {
			int t;
			t = read(sock, buf, 100);

			if (t == 0) {
				close(sock);
				count--;
			} else {
				write(1, buf, t);
			}
		} else {
			die("????");
		}
	}

	close(epfd);

	return 0;
}

int create_sock(char *host, char *port) {
	int sock, ret;
	struct sockaddr_in addr;
	struct addrinfo *gai, hints;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		die("socket");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	/*
	 * Consultamos la información sobre la dirección que nos
	 * dieron. Podemos pasar una IP, o un nombre que será
	 * consultado a /etc/hosts o al nameserver configurado
	 */
	ret = getaddrinfo(host, port, &hints, &gai);
	if (ret)
		die("getaddrinfo (%s)", gai_strerror(ret));

	/*
	 * getaddrinfo devuelve una lista enlazada con
	 * información, tomamos el primer nodo
	 */

	addr = *(sin*)gai->ai_addr;

	freeaddrinfo(gai);

	/* Conectamos a esa dirección */
	ret = connect(sock, (sad*)&addr, sizeof addr);
	if (ret < 0)
		die("connect");

	return sock;
}

int setup_epoll(int sockfd) {
	int epfd;
	int ret;
	struct epoll_event ev;

	epfd = epoll_create(1);
	if (epfd < 0)
		die("epoll_create");

	ev.events = EPOLLIN;
	ev.data.fd = sockfd;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	if (ret)
		die("epoll_ctl.1");

	ev.events = EPOLLIN;
	ev.data.fd = 0;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);
	if (ret)
		die("epoll_ctl.2");

	return epfd;
}

