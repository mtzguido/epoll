#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common.h"

struct fdinfo {
	enum {
		LSOCK,
		CLIENT
	} type;
	int fd;
	struct sockaddr_in sin;
};

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

int start_listener(int port);
void service(int epfd);

int main () {
	int epfd = epoll_create(1);
	int lsock = start_listener(9000);
	struct epoll_event ev;
	struct fdinfo *fdinfo;
	int ret;

	fdinfo = malloc(sizeof (struct fdinfo));
	fdinfo->type = LSOCK;
	fdinfo->fd = lsock;

	ev.data.ptr = fdinfo;
	ev.events = EPOLLIN;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lsock, &ev);
	if (ret < 0)
		die("epoll_ctl.1");

	service(epfd);

	close(epfd);

	return 0;
}

int start_listener(int port) {
	struct sockaddr_in listen_addr;
	int lsock;
	int ret;

	lsock = socket(AF_INET, SOCK_STREAM, 0);
	if (lsock < 0)
		die("socket");

	memset(&listen_addr, 0, sizeof listen_addr);
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(lsock, (sad*)&listen_addr, sizeof listen_addr);
	if (ret < 0)
		die("bind");

	ret = listen(lsock, 10);
	if (ret < 0)
		die("listen");

	return lsock;
}

void accept_one(int epfd, int lsock) {
	int cli;
	struct fdinfo *fdinfo;
	struct epoll_event ev;
	socklen_t crap = sizeof fdinfo->sin;
	int ret;

	fdinfo = malloc(sizeof *fdinfo);

	cli = accept4(lsock, (sad*)&fdinfo->sin, &crap, SOCK_NONBLOCK);
	if (cli < 0)
		die("accept");

	fdinfo->fd = cli;
	fdinfo->type = CLIENT;

	ev.data.ptr = fdinfo;
	ev.events = EPOLLIN | EPOLLRDHUP;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cli, &ev);
	if (ret < 0)
		die("epoll_ctl.2");
}

void handle_cli(struct fdinfo *fdinfo) {
	int fd = fdinfo->fd;
	int t;
	char buf1[100], buf2[100];

	inet_ntop(AF_INET, &fdinfo->sin.sin_addr, buf1, sizeof fdinfo->sin);

	do {
		t = read(fd, buf2, 100);
		buf2[t] = 0;
		printf("%s mandó %s\n", buf1, buf2);
		write(fd, buf2, t);
	} while (t > 0);
}

void kill_cli(int epfd, struct fdinfo *fdinfo) {
	char buf[100];
	int fd = fdinfo->fd;
	free(fdinfo);

	inet_ntop(AF_INET, &fdinfo->sin.sin_addr, buf, sizeof fdinfo->sin);

	printf("Chau %s!\n", buf);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

void handle_event(int epfd, struct epoll_event ev) {
	struct fdinfo *fdinfo = ev.data.ptr;

	switch (fdinfo->type) {
	case LSOCK:
		accept_one(epfd, fdinfo->fd);
		break;
	case CLIENT:
		print_event(fdinfo->fd, ev);

		if (ev.events & EPOLLRDHUP)  {
			handle_cli(fdinfo);
			kill_cli(epfd, fdinfo);
		} else if (ev.events & EPOLLIN) {
			handle_cli(fdinfo);
		}

		break;
	}
}

void service(int epfd) {
	int i, n;
	struct epoll_event evs[10];

	for (;;) {
		n = epoll_wait(epfd, evs, 10, 5000);

		if (n < 0)
			die("epoll_wait");

		if (n == 0) {
			printf("Que aburrimiento che....\n");
			continue;
		}

		for (i=0; i<n; i++)
			handle_event(epfd, evs[i]);
	}
}
