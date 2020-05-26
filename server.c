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

struct Socket
{
	int fd_in, fd_out, is_alive, weight;
};


int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("Wrong arguments\n");

		return 0;
	}
	int n = strtol(argv[1], NULL, 10);

	int i = 0;


	struct Socket* arr_fd = calloc(n, sizeof(struct Socket));
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
	memset(&s_send, 0, sizeof(s_send));
	s_send.sin_family = AF_INET;
	s_send.sin_port = htons(port);
	s_send.sin_addr.s_addr = INADDR_ANY;


	int lst_sk = 0;
	lst_sk = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(lst_sk, SOL_SOCKET, SO_REUSEADDR, &broadcastFl, sizeof(broadcastFl));
	bind(lst_sk, (struct sockaddr*) &s_send, sizeof(s_send));
	listen(lst_sk, 1000);




	int threads = 0;

	for(i = 0; i < n; ++i)
	{
		int l = 0;
		broadcastFl = 1;


		struct sockaddr_in s_work;
		int len = sizeof(s_work);



		arr_fd[i].fd_in = accept(lst_sk, (struct sockaddr*) &s_work, &len);
		s_work.sin_port = port;

		setsockopt(arr_fd[i].fd_in, SOL_SOCKET, SO_KEEPALIVE, &broadcastFl, sizeof(broadcastFl));
		int intvl = 5;
		setsockopt(arr_fd[i].fd_in, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
		int probes = 2;
		setsockopt(arr_fd[i].fd_in, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(int));
		int timeout = 20;
		setsockopt(arr_fd[i].fd_in, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(int));


		shutdown(arr_fd[i].fd_in, SHUT_WR);
		perror("Set");

		arr_fd[i].fd_out = socket(AF_INET, SOCK_STREAM, 0);
		connect(arr_fd[i].fd_out, (struct sockaddr*) &s_work, len);
		perror("Set");

		setsockopt(arr_fd[i].fd_out, SOL_SOCKET, SO_KEEPALIVE, &broadcastFl, sizeof(broadcastFl));
		setsockopt(arr_fd[i].fd_out, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
		setsockopt(arr_fd[i].fd_out, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(int));
		setsockopt(arr_fd[i].fd_out, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(int));		

		shutdown(arr_fd[i].fd_out, SHUT_RD);


		read(arr_fd[i].fd_in, &arr_fd[i].weight, sizeof(int));

		threads += arr_fd[i].weight;
	}


	double next = 0;
	for(i = 0; i < n; ++i)
	{
		struct Task t;
		t.start = next;
		t.fin = next + 3.0 * arr_fd[i].weight / threads;
		next += 3.0 * arr_fd[i].weight / threads;

		//printf("%f\n", t.fin);

		write(arr_fd[i].fd_out, &t, sizeof(t));


		arr_task[i] = i;
		arr_fd[i].is_alive = LIVE;
	}

	fd_set srd;

	int counter = 0; // used to count complete tasks
	double res = 0;

	while(counter != n)
	{
		for (i = 0; i < n; ++i)
		{
			if (arr_fd[i].is_alive == DEAD) 
			{
				printf("SKIP\n");
				continue;
			}
			double ans;
			int result = read(arr_fd[i].fd_in, &ans, sizeof(ans));
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
							//printf("%d\n", j);
							if (arr_fd[j].is_alive == LIVE)
							{
								double next = 0;
								int ii = 0;
								fl = 1;
								for (ii = 0; ii < l; ++ii)
									next += 3.0 * arr_fd[ii].weight / threads;


								struct Task t;
								t.start = next;
								t.fin = t.start + 3.0 * arr_fd[l].weight / threads; // weigth of the i'th task is same as weight of i'th fd
								write(arr_fd[j].fd_out, &t, sizeof(t));
								printf("write %d\n", i);
								arr_task[l] = j;
								break; 
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
			printf("**%d\n", i);
		}
	}
	shutdown(lst_sk, SHUT_RDWR);


/**/
	for(i = 0; i < n; i++)
	{
		shutdown(arr_fd[i].fd_in, SHUT_RD);
		shutdown(arr_fd[i].fd_out, SHUT_WR);
	}

	printf("%.3f\n", res);





	return 0;
}




















