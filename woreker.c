#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <math.h>
#include <errno.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define N_CORES 6

struct CalcStr
{
	double st, fin, delta, cur, res;
	double* final_res;
	int sem_id, num_af;
	char trash[64];
};


struct Task
{
	double start, fin;
};


struct Useless
{
	int num_af, sem_id;	
};


void* calculate(void* ptr)
{
	struct CalcStr* mem = (struct CalcStr*) ptr;

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET((mem->num_af), &set);
	sched_setaffinity(0, sizeof(set), &set);

	//affinity




	for (mem->cur = mem->st; mem->cur < mem->fin; mem->cur += mem->delta)
	{
		mem->res += (pow(mem->cur, 3) + pow(mem->cur + mem->delta, 3)) * mem->delta / 2;
	} 

	struct sembuf str;

	str.sem_num = 0;
	str.sem_op = -1;
	str.sem_flg = 0;

	semop(mem->sem_id, &str, 1);

	*mem->final_res += mem->res; 

	str.sem_num = 0;
	str.sem_op = 1;
	str.sem_flg = 0;

	semop(mem->sem_id, &str, 1);
	return NULL;
}


void* useless(void* smth)
{	
	struct Useless* arg = smth;

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET((arg->num_af), &set);
	sched_setaffinity(0, sizeof(set), &set);

//affinity

	struct sembuf buf[2];

	buf[0].sem_num = 1;
	buf[0].sem_op = -1;
	buf[0].sem_flg = IPC_NOWAIT;

	buf[1].sem_num = 1;
	buf[1].sem_op = 1;
	buf[1].sem_flg = 0;

	while(semop(arg->sem_id, buf, 2) == -1);

	return NULL;
}




/*x^3 [1, 3]*/
int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("Wrong arguments\n");
		return 0;
	}

	int n = strtol(argv[1], NULL, 10);
	if (errno == ERANGE)
	{
		printf("Error create threads\n");
		exit(EXIT_FAILURE);
	}







	//+++++++++++++++++++++++++++++++++++++++



	int broad = socket(AF_INET, SOCK_DGRAM, 0);

	int broadcatFl = 1;
	setsockopt(broad, SOL_SOCKET, SO_BROADCAST, &broadcatFl, sizeof(broadcatFl));
	setsockopt(broad, SOL_SOCKET, SO_REUSEADDR, &broadcatFl, sizeof(broadcatFl));



	struct sockaddr_in s_work, s_send;
	memset(&s_work, 0, sizeof(s_work));
	s_work.sin_family = AF_INET;
	s_work.sin_port = htons(50000);
	s_work.sin_addr.s_addr = INADDR_ANY;

	bind(broad, (struct sockaddr*) &s_work, sizeof(s_work));

	memset(&s_send, 0, sizeof(s_send));
	int len = sizeof(s_send);
	int port;
	recvfrom(broad, &port, sizeof(int), 0, (struct sockaddr*) &s_send, &len);
	close(broad);

	perror("recv");

	s_send.sin_family = AF_INET;
	s_send.sin_port = htons(port);











	int sk_out = socket(AF_INET, SOCK_STREAM, 0);
	perror("fcntl");
	connect(sk_out, (struct sockaddr*) &s_send, len);
	perror("connect");

	int broadcastFl = 1;
	setsockopt(sk_out, SOL_SOCKET, SO_KEEPALIVE, &broadcastFl, sizeof(broadcastFl));
	int intvl = 5;
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
	int probes = 2;
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(int));
	int timeout = 20;
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(int));

	shutdown(sk_out, SHUT_RD);
	write(sk_out, &n, sizeof(n));




	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(s_send));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;


	int lst_sk = 0;
	lst_sk = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(lst_sk, SOL_SOCKET, SO_REUSEADDR, &broadcastFl, sizeof(broadcastFl));
	bind(lst_sk, (struct sockaddr*) &addr, sizeof(addr));
	listen(lst_sk, 1000);
	perror("listen");

	int sk_in = accept(lst_sk, NULL, NULL);
	setsockopt(sk_out, SOL_SOCKET, SO_KEEPALIVE, &broadcastFl, sizeof(broadcastFl));
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(int));
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(int));
	setsockopt(sk_out, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(int));


	shutdown(sk_in, SHUT_WR);








	struct Task t;
	



	//++++++++++++++++++++

	while(read(sk_in, &t, sizeof(t)) != 0)
	{
		printf("%f\n", t.fin);

		struct CalcStr* mem = (struct CalcStr*) calloc (2 * n, sizeof(struct CalcStr));
		if (!mem)
		{
			printf("Error create threads\n");
			exit(EXIT_FAILURE);
		}



		int i = 0;

		int max = n > N_CORES ? n : N_CORES;

		pthread_t* tinfo = (pthread_t*) calloc (max, sizeof(pthread_t));
		if (!tinfo)
		{
			printf("Error create threads\n");
			exit(EXIT_FAILURE);
		}


		double step = (t.fin - t.start) / n;
		double start = t.start;
		double res = 0;
		double delta = 2.0 / 10000000; //0


		sleep(3);


		int sem_id = semget(IPC_PRIVATE, 2, 0666);

		struct sembuf str;

		str.sem_num = 0;
		str.sem_op = 1;
		str.sem_flg = 0;

		semop(sem_id, &str, 1);

		for (i = 0; i < n; ++i, start += step)
		{
			mem[2 * i].st = start;
			mem[2 * i].fin = start + step;
			mem[2 * i].num_af = i % N_CORES;
			mem[2 * i].sem_id = sem_id;
			mem[2 * i].final_res = &res;
			mem[2 * i].delta = delta;
			if (pthread_create(&tinfo[i], NULL, calculate,(void*) &(mem[i * 2])) != 0)
			{
				printf("Error create threads\n");
				exit(EXIT_FAILURE);
			}
		}


		struct Useless buf[N_CORES];



		if (n < N_CORES)
		{
			for (i = 0; i < N_CORES; ++i)
			{
				buf[i].sem_id = sem_id;
				buf[i].num_af = i;
			}

			for (i = n; i < N_CORES; ++i)
			{
				if (pthread_create(&tinfo[i], NULL, useless, (void*) &buf[i]) == -1)
				{
					printf("Error create threads\n");
					exit(EXIT_FAILURE);
				}
			}
		}

		for (i = 0; i < n; ++i)
		{
			if (pthread_join(tinfo[i], NULL) != 0)
			{
				printf("Error join thread: %d %d\n", i, errno);
				exit(EXIT_FAILURE);
			}
		}

		str.sem_num = 1;
		str.sem_op = 1;
		str.sem_flg = 0;

		semop(sem_id, &str, 1);

		write(sk_out, &res, sizeof(res));
		printf("%.3f\n", res);

		free(tinfo);
		free(mem);
	}

	shutdown(sk_out, SHUT_WR);
	shutdown(sk_in, SHUT_RD);
	return 0;
}
