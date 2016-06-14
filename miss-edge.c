#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

static void print_event (struct epoll_event ev) {
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
			ev.data.fd, flags_str+1);
}

int main() {
	int epfd;
	struct epoll_event ev;
	struct epoll_event ret;
	char buf[200];
	int n, t;

	epfd = epoll_create(1);

	ev.data.fd = 0;
	ev.events = EPOLLIN | EPOLLET;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev) != 0)
		perror("epoll_ctl");

	while ((n = epoll_wait(epfd, &ret, 1, -1)) > 0) {
		printf("tick!\n");
		print_event(ret);

		continue;

		if(ret.data.fd == 0) {
			t = read(0, buf, 100);

			if (t == 0) {
				close(0);
				printf("stdin done\n");
			}
		}
	}

	return 0;
}
