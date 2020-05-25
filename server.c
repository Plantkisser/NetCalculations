#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <signal.h>
#include <netinet/tcp.h>





#define DEAD 23
#define LIVE 42

#define DONE -23



struct Task
{
	double start, fin;
};

struct Fd
{
	int fd, is_alive;
};


int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("Wrong arguments\n");
	}
	int n = strtol(argv[1], NULL, 10);

	int i = 0;


	struct Fd* arr_fd = calloc(n, sizeof(struct Fd));
	if (arr_fd == NULL)
	{
		printf("Error creating array\n");
		return 0;
	}

	int* arr_task = calloc(n, sizeof(int));
	if (arr_task == NULL)
	{
		printf("Error creating array\n");
		return 0;
	}




	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);



	int broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
	int broadcastFl = 1;
	setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, &broadcastFl, sizeof(broadcastFl));


	struct sockaddr_in s_broad;
	memset(&s_broad, 0, sizeof(s_broad));
	s_broad.sin_family = AF_INET;
	s_broad.sin_port = htons(50000);
	s_broad.sin_addr.s_addr = INADDR_BROADCAST;




	int port = 50000;
	sendto(broadcast_fd, &port, sizeof(port), 0, (struct sockaddr*) &s_broad, sizeof(s_broad));



	close(broadcast_fd);




	struct sockaddr_in s_send;
	memset(&s_broad, 0, sizeof(s_send));
	s_send.sin_family = AF_INET;
	s_send.sin_port = htons(port);
	s_send.sin_addr.s_addr = INADDR_ANY;


	int arr_sk = 0;
	arr_sk = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(arr_sk, SOL_SOCKET, SO_REUSEADDR, &broadcastFl, sizeof(broadcastFl));
	bind(arr_sk, (struct sockaddr*) &s_send, sizeof(s_send));
	listen(arr_sk, 1000);



	int nfds = 0;

	for(i = 0; i < n; ++i)
	{
		int l = 0;
		broadcastFl = 1;
		arr_fd[i].fd = accept(arr_sk, NULL, NULL);
		setsockopt(arr_fd[i].fd, SOL_SOCKET, SO_KEEPALIVE, &broadcastFl, sizeof(broadcastFl));
		int intvl = 5;
		setsockopt(arr_fd[i].fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
		int probes = 2;
		setsockopt(arr_fd[i].fd, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(int));
		int timeout = 20;
		setsockopt(arr_fd[i].fd, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(int));
		struct Task t;
		t.start = 3.0 / n * i;
		t.fin = t.start + 3.0 / n;

		//printf("%f\n", t.fin);

		write(arr_fd[i].fd, &t, sizeof(t));


		nfds = nfds > arr_fd[i].fd? nfds : arr_fd[i].fd;
		arr_task[i] = i;
		arr_fd[i].is_alive = LIVE;
	}
	perror("Set");

	fd_set srd;

	int counter = 0;
	double res = 0;

	while(counter != n)
	{
		for (i = 0; i < n; ++i)
		{
			if (arr_fd[i].is_alive == DEAD) continue;
			double ans;
			int result = read(arr_fd[i].fd, &ans, sizeof(ans));
			if (result == -1 || result == 0)
			{
				arr_fd[i].is_alive = DEAD;
				int l = 0;
				for (l = 0; l < n; l++)
				{
					if (i == arr_task[l])
					{
						int j = 0;
						int fl = 0;
						for (j = 0; j < n; ++j)
						{
							if (arr_fd[j].is_alive == LIVE)
							{
								struct Task t;
								t.start = 3.0 / n * i;
								t.fin = t.start + 3.0 / n;
								write(arr_fd[j].fd, &t, sizeof(t));
								arr_task[l] = j; 
							}
						}
						if (fl == 0)
						{
							printf("All workers are disconnected\n");
							return 0;
						}
					}
				}
			}
			else
			{
				counter++;
				res += ans;
			}
			printf("***\n");
		}
	}
	shutdown(arr_sk, SHUT_RDWR);


/**/
	for(i = 0; i < n; i++)
	{
		shutdown(arr_fd[i].fd, SHUT_RDWR);
	}

	printf("%.3f\n", res);





	return 0;
}




















