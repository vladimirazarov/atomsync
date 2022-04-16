#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// queue type
typedef struct
{
	int len;
	int *data;
} queue;

// Functions headers
void validateInput(int argc, char **argv);
int isDigit(char *str);
void queuePush(queue *q, int element);
int queuePop(queue *q, int type);
void queueCtr(queue *q);
queue *queueDtr(queue *q);

void queuePrint(queue *q);

// Global variables
sem_t *semInit = NULL;
sem_t *semMol = NULL;
sem_t *semQ = NULL;

// out .. ptr to var
// create shared mem variable
#define SHARED_MEM_INT(count) mmap(NULL, sizeof(int) * (count), \
								   PROT_READ | PROT_WRITE,      \
								   MAP_SHARED | MAP_ANONYMOUS, -1, 0);

#define SHARED_MEM_QUEUE(size) mmap(NULL, sizeof(queue) + sizeof(int) * (size), \
									PROT_READ | PROT_WRITE,                     \
									MAP_SHARED | MAP_ANONYMOUS, -1, 0);

#define randInt(min, max) (rand() % (max - min + 1) + min)
#define FILENAME "sema.c"

#define SEM_INIT_FILENAME "/semInit"
#define SEM_MOLECULE_FILENAME "/semMol"
#define SEM_QUEUE_FILENAME "/semQ"

int main(int argc, char **argv)
{
	/*
	argv[0] ... program
	argv[1] ... oxygen number ... int NO ... >=0
	argv[2] ... hydrogen number ... int NH ... >=0
	argv[3] ... max time to wait before hydrogen or oxygen will go to the queue ... int TI ... 0<=TI<=1000(0-10 seconds)
	argv[4] ... max time for creating 1 molecule ... int TB ... 0<=TB<=1000
	*/
	validateInput(argc, argv);
	int NO = atoi(argv[1]);
	int NH = atoi(argv[2]);
	int TI = atoi(argv[3]);
	int TB = atoi(argv[4]);
	pid_t pid;

	srand((unsigned)time(NULL));

	int *actionN = SHARED_MEM_INT(1);
	*actionN = 1;

	int *oxyCurID = SHARED_MEM_INT(1);
	*oxyCurID = 1;

	int *hydroCurID = SHARED_MEM_INT(1);
	*hydroCurID = 1;

	int *noM = SHARED_MEM_INT(1);
	*noM = 1;

	// index is the process num, value is the status
	// 0 ... initialized and places into queue
	// 1 ... in process of creating molecule
	// 2 ... used in molecule creating
	int *oxyStatus = SHARED_MEM_INT(NO + 1);
	int *hydroStatus = SHARED_MEM_INT(NH + 1);

	int *forkingO = SHARED_MEM_INT(1);
	int *forkingH = SHARED_MEM_INT(1);

	queue *oxyQ = SHARED_MEM_QUEUE(NO);
	queue *hydroQ = SHARED_MEM_QUEUE(NH);
	queueCtr(oxyQ);
	queueCtr(hydroQ);

	sem_unlink(SEM_INIT_FILENAME);
	sem_unlink(SEM_MOLECULE_FILENAME);
	sem_unlink(SEM_QUEUE_FILENAME);

	semInit = sem_open(SEM_INIT_FILENAME, O_CREAT, 0660, 1);
	if (semInit == SEM_FAILED)
	{
		perror("sem_open/semInit");
		exit(EXIT_FAILURE);
	}

	semMol = sem_open(SEM_MOLECULE_FILENAME, O_CREAT, 0660, 1);
	if (semMol == SEM_FAILED)
	{
		perror("sem_open/semMol");
		exit(EXIT_FAILURE);
	}

	semQ = sem_open(SEM_QUEUE_FILENAME, O_CREAT, 0660, 1);
	if (semQ == SEM_FAILED)
	{
		perror("sem_open/semQ");
		exit(EXIT_FAILURE);
	}

	// printf("%d: O %d: creating molecule %d\n", *actionN, poppedO, *noM);
	// printf("%d: O %d: started\n", *actionN, idO);
	// printf("%d: O %d: going to queue\n", *actionN, idO);
	// printf("%d: O %d: molecule %d created\n", *actionN, poppedO, *noM);

	pid = fork();
	// Process forked from main process that will initialaze oxygen atoms
	if (pid == 0)
	{
		*forkingO = true;
		for (int i = 0; i < NO; i++)
		{
			pid = fork();
			if (pid == 0)
			{
				sem_wait(semInit);
				// Initialize idO of this process
				int idO = *oxyCurID;
				printf("%d: O %d: started\n", *actionN, idO);

				// Will change shared variables, use semaphore
				// Increment shared actionN
				(*actionN)++;
				// Increment shared oxyCurID
				(*oxyCurID)++;
				// Update status
				oxyStatus[idO] = 0;
				sem_post(semInit);

				// sleep
				usleep(randInt(0, TI));

				// changing shared queue and status, use semaphore
				sem_wait(semInit);
				printf("%d: O %d: going to queue\n", *actionN, idO);
				queuePush(oxyQ, idO);
				sem_post(semInit);
				while(1);
				exit(0);
			}
			else
				printf("this ps %d, parent %d\n", getpid(), getppid());
		}
		*forkingO = false;
	}
	// Main process resumed, fork process that will initialize hydrogen atoms
	else
	{
		pid = fork();
		if (pid == 0)
		{
			*forkingH = true;
			for (int i = 0; i < NH; i++)
			{
				pid = fork();
				if (pid == 0)
				{
					sem_wait(semInit);
					printf("Created H\n");
					sem_post(semInit);
					while(1);
					exit(1);
				}
			}
			*forkingH = false;
		}
		else
		{
			while(1);
			while (wait(NULL) > 0 || *forkingH == true || *forkingO == true)
				;
			queuePrint(oxyQ);
			exit(0);
		}
	}
}

/*
   Return 1 if string is full of digits.
   Otherwise return 0 if at least one letter
   or other non-digit character is met
*/
int isDigit(char *str)
{
	int i;
	for (i = 0; str[i] != '\0'; i++)
	{
		if (str[i] < '0' || str[i] > '9')
		{
			if (i == 0 && str[i] == '-')
				continue;
			return 0;
		}
	}
	return 1;
}

/*
   Validates input command line arguments
   process is exited with exit code 1 if
   arguments are invalid. Otherwise do
   nothing
*/
void validateInput(int argc, char **argv)
{
	if (argc != 5)
	{
		fprintf(stderr, "Invalid number of arguments\n");
		exit(EXIT_FAILURE);
	}

	if (!isDigit(argv[1]) || !isDigit(argv[2]) || !isDigit(argv[3]) || !isDigit(argv[4]))
	{
		fprintf(stderr, "All arguments must be numbers!\n");
		exit(EXIT_FAILURE);
	}

	int NO = atoi(argv[1]);
	if (NO < 0)
	{
		fprintf(stderr, "Invalid number of oxygen\n");
		printf("%d\n", NO);
		exit(EXIT_FAILURE);
	}

	int NH = atoi(argv[2]);
	if (NH < 0)
	{
		fprintf(stderr, "Invalid number of hydrogen\n");
		exit(EXIT_FAILURE);
	}

	int TI = atoi(argv[3]);
	if (TI < 0 || TI > 1000)
	{
		fprintf(stderr, "Invalid maximum idle time\n");
		exit(1);
	}

	int TB = atoi(argv[4]);
	if (TB < 0 || TB > 1000)
	{
		fprintf(stderr, "Invalid maximum time required for molecule creating\n");
		exit(1);
	}
}

void queueCtr(queue *q)
{
	q->len = 0;
	q->data = (int *)q + sizeof(int);
}

void queuePush(queue *q, int element)
{
	q->len++;

	for (int i = q->len; i > 0; i--)
		q->data[i] = q->data[i - 1];

	q->data[0] = element;
}

void queuePrint(queue *q)
{
	for (int i = 0; i < q->len; i++)
	{
		printf("%d ", q->data[i]);
	}
	putchar('\n');
}

// Pop an element from the end of the queue
// Return its value
// if len is lesser than 2 (not enough hydrogen), return -2
// if len is lesser than 1 (not enough oxygen), return -1
// type ... 1 - oxyQ,
// type ... 2 - hydroQ
int queuePop(queue *q, int type)
{
	if (q->len < 2 && type == 2)
		return -2;
	if (q->len < 1 && type == 1)
		return -1;

	int popped = q->data[q->len - 1];
	q->len--;
	return popped;
}

queue *queueDtr(queue *q)
{
	free(q->data);
}