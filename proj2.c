/**************************************************************************
  * Project name: IOS project 2
  * File: proj2.c
  * Date: 4/17/2022
  * Last change: 4/17/2022
  * Author: Vladimir Azarov xazaro00@vutbr.cz
  * Author's university login: xazaro00
  *
  * Description: Second semester VUT FIT operating system class project.
  * Main concepts: IPC, shared memory, semaphores, file handling, 
  * process synchronization.
  *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// Queue type
typedef struct
{
	// Length of the queue
	int len;
	// Data pointer, an array of integers 
	int *data;
} queue;

// Functions headers
void validateInput(int argc, char **argv);
int isDigit(char *str);
void queuePush(queue *q, int element);
int queuePop(queue *q);
void queueCtr(queue *q);
queue *queueDtr(queue *q);
void queuePrint(queue *q);

// semInit semaphore manages oxyCurId, hydroCurId and actionN variables.
sem_t *semInit = NULL;
// semMol and semCreating semaphores manage molecule creating state.
sem_t *semMol = NULL;
sem_t *semCreating = NULL;
// semQ semaphore manages popping and pushing elemnts to/from queues. 
sem_t *semQ = NULL;
FILE *fOutPtr;

// Shared memory integer, parameter count defines number of integers to allocate.
#define SHARED_MEM_INT(count) mmap(NULL, sizeof(int) * (count), \
								   PROT_READ | PROT_WRITE,      \
								   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
// Shared memory queue, parameter count defines maximum number of integers queue can contain.
#define SHARED_MEM_QUEUE(size) mmap(NULL, sizeof(queue) + sizeof(int *) + sizeof(int) * (size), \
									PROT_READ | PROT_WRITE,                                     \
									MAP_SHARED | MAP_ANONYMOUS, -1, 0);
// Random integer generator macro.
#define randInt(min, max) (rand() % (max - min + 1) + min)

// Semaphores names strings.
#define SEM_INIT_FILENAME "/semInit.xazaro00"
#define SEM_MOLECULE_FILENAME "/semMol.xazaro00"
#define SEM_QUEUE_FILENAME "/semQ.xazaro00"
#define SEM_CREATING_FILENAME "/semCreating.xazaro00"

int main(int argc, char **argv)
{
	// Validate input function, all arguments must meet following requirements:
	/*
	argv[1] ... oxygen number. int NO >=0
	argv[2] ... hydrogen number. int NH >=0
	argv[3] ... max time to wait before hydrogen or oxygen will go to the queue.
	int TI. 0<=TI<=1000
	argv[4] ... max time for creating 1 molecule. int TB. 0<=TB<=1000
	*/
	validateInput(argc, argv);
	// If input is validated and no errors occurred, store arguments.
	int NO = atoi(argv[1]);
	int NH = atoi(argv[2]);
	int TI = atoi(argv[3]);
	int TB = atoi(argv[4]);
	// pid helping variable.
	pid_t pid;
	// Open the output file for writing.
	fOutPtr = fopen("proj2.out", "w");
	// Tell program not to use buffer for the synchronization purposes.
	setbuf(fOutPtr, NULL);
	// Set up the random
	srand((unsigned)time(NULL));
	// The number of actions, shared variable.
	int *actionN = SHARED_MEM_INT(1);
	*actionN = 1;
	// Current oxygen atoms ID, shared variable.
	int *oxyCurID = SHARED_MEM_INT(1);
	*oxyCurID = 1;
	// Current hydrogen atoms ID, shared variable.
	int *hydroCurID = SHARED_MEM_INT(1);
	*hydroCurID = 1;
	// Current number of the molecule being created, shared variable.
	int *noM = SHARED_MEM_INT(1);
	*noM = 1;
	/* Molecules status arrays. Shared array.
	   Index is the process ID (idO or IDH), value is the signal.
	   Values on indexies and their meaning:
	 0 - no signal was sent, array is initialized.
	 1 - signal for hydrogen atoms to print "Molecule created".
	 2 - signal for oxygen and hydrogen atoms to print "Creating molecule".
	 4 - signal for oxygen sent by hydrogen that hydrogen has ended printing
		 "Molecule creating" and done all required operations after that.
	 5 - signal for oxygen sent by hydrogen that hydrogen has ended printing
		 "Molecule created" and done all required operations after that.
	*/
	int *oxyStatus = SHARED_MEM_INT(NO + 1);
	int *hydroStatus = SHARED_MEM_INT(NH + 1);
	for (int i = 1; i < NH + 1; i++)
	{
		hydroStatus[i] = 0;
		oxyStatus[i] = 0;
	}
	// Semantically bool shared variable that signalizes, that
	// molecule is being created, shared variable.
	int *molBeingCreat = SHARED_MEM_INT(1);
	*molBeingCreat = false;
	// Counter that counts how many hydrogens atoms we used so far
	// (to check if we dont have enough left), shared variable.
	int *hydrosUsed = SHARED_MEM_INT(1);
	*hydrosUsed = 0;
	// Counter that counts how many oxygen atoms we used so far
	// (to check if we dont have enough left), shared variable.
	int *oxysUsed = SHARED_MEM_INT(1);
	*oxysUsed = 0;
	// Currenty popped oxygen and 2 hydrogen atoms, that are
	// participating in creating a molecule, shared variables.
	int *poppedH1 = SHARED_MEM_INT(1);
	int *poppedH2 = SHARED_MEM_INT(1);
	int *poppedO = SHARED_MEM_INT(1);
	// Queues of oxygens and hydrogens, shared data structures.
	queue *oxyQ = SHARED_MEM_QUEUE(NO);
	queue *hydroQ = SHARED_MEM_QUEUE(NH);
	queueCtr(oxyQ);
	queueCtr(hydroQ);
	// Semaphores unlinks.
	sem_unlink(SEM_INIT_FILENAME);
	sem_unlink(SEM_MOLECULE_FILENAME);
	sem_unlink(SEM_QUEUE_FILENAME);
	sem_unlink(SEM_CREATING_FILENAME);

	// Semaphores initialization.
	semInit = sem_open(SEM_INIT_FILENAME, O_CREAT, 0660, 1);
	if (semInit == SEM_FAILED)
	{
		fprintf(stderr, "ERROR: Couldn't initialize semaphore.\n");
		exit(EXIT_FAILURE);
	}
	semMol = sem_open(SEM_MOLECULE_FILENAME, O_CREAT, 0660, 1);
	if (semMol == SEM_FAILED)
	{
		fprintf(stderr, "ERROR: Couldn't initialize semaphore.\n");
		exit(EXIT_FAILURE);
	}
	semQ = sem_open(SEM_QUEUE_FILENAME, O_CREAT, 0660, 1);
	if (semQ == SEM_FAILED)
	{
		fprintf(stderr, "ERROR: Couldn't initialize semaphore.\n");
		exit(EXIT_FAILURE);
	}
	semCreating = sem_open(SEM_CREATING_FILENAME, O_CREAT, 0660, 1);
	if (semCreating == SEM_FAILED)
	{
		fprintf(stderr, "ERROR: Couldn't initialize semaphore.\n");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	// Process forked from main process that will initialize oxygen atoms
	if (pid == 0)
	{
		for (int i = 0; i < NO; i++)
		{
			pid = fork();
			if (pid == 0)
			{
				// Will change shared variables, use semaphore
				sem_wait(semInit);
				// Initialize idO of this process
				int idO = *oxyCurID;
				fprintf(fOutPtr, "%d: O %d: started\n", *actionN, idO);
				// Increment shared actionN
				(*actionN)++;
				// Increment shared oxyCurID
				(*oxyCurID)++;
				sem_post(semInit);

				// sleep, while sleeping other processes can come into action
				usleep(randInt(0, TI));

				// changing shared queue and status, use semaphore
				sem_wait(semInit);
				fprintf(fOutPtr, "%d: O %d: going to queue\n", *actionN, idO);
				(*actionN)++;
				sem_post(semInit);

				// push into queue, using semaphore semQ
				sem_wait(semQ);
				queuePush(oxyQ, idO);
				sem_post(semQ);

				// loop in which process is waiting for signals
				// oxygen processes also manage molecule creating state
				while (1)
				{
					// If we dont have enough H left
					if ((NH - *hydrosUsed) < 2)
					{
						sem_wait(semInit);
						fprintf(fOutPtr, "%d: O %d: not enough H\n", *actionN, idO);
						(*actionN)++;
						sem_post(semInit);
						exit(EXIT_SUCCESS);
					}

					// creating molecule O state
					if (oxyStatus[idO] == 2)
					{
						sem_wait(semInit);
						fprintf(fOutPtr, "%d: O %d: creating molecule %d\n", *actionN, idO, *noM);
						(*actionN)++;
						sem_post(semInit);
						// wait for signal from hydrogen atoms that state "creating molecule" was processed 
						while ((hydroStatus[*poppedH1] != 4) || (hydroStatus[*poppedH2] != 4));
						// creating molecule sleep
						usleep(randInt(0, TB));
						// send signal for hydrogen to print "molecule created"
						hydroStatus[*poppedH1] = 1;
						hydroStatus[*poppedH2] = 1;

						// wait for signal from hydrogen atoms that state "molecule created" was processed 
						while ((hydroStatus[*poppedH1] != 5) || (hydroStatus[*poppedH2] != 5))
							;
						*hydrosUsed = *hydrosUsed + 2;
						*oxysUsed = *oxysUsed + 1;
						sem_wait(semInit);
						fprintf(fOutPtr, "%d: O %d: molecule %d created\n", *actionN, idO, *noM);
						(*actionN)++;
						(*noM)++;
						sem_post(semInit);
						*molBeingCreat = false;
						sem_post(semCreating);
						exit(EXIT_SUCCESS);
					}

					// Use semaphore, so 2 oxygen atoms cant simultaneously be 
					// inside of this if statement
					sem_wait(semMol);
					if (((oxyQ->len) >= 1) && ((hydroQ->len) >= 2) && *molBeingCreat == false)
					{
						*molBeingCreat = true;
						sem_wait(semCreating);

						*poppedO = queuePop(oxyQ);
						// signal for oxygen to go into state "creating molecule"
						oxyStatus[*poppedO] = 2;

						*poppedH1 = queuePop(hydroQ);
						// signal for hydrogen to go into state "creating molecule"
						hydroStatus[*poppedH1] = 2;

						*poppedH2 = queuePop(hydroQ);
						// signal for hydrogen to go into state "creating molecule"
						hydroStatus[*poppedH2] = 2;
					}
					sem_post(semMol);
				}
			}
			else if (pid == -1)
			{
				fprintf(stderr, "ERROR: Couldn't fork\n");
				exit(EXIT_FAILURE);
			}
		}
		// process that creates oxygen atoms is waiting for it's 
		// children to end
		while (wait(NULL) > 0)
			;
		exit(EXIT_SUCCESS);
	}
	else if (pid == -1)
	{
		fprintf(stderr, "ERROR: Couldn't fork\n");
		exit(EXIT_FAILURE);
	}
	// Main process resumed, fork process that will initialize hydrogen atoms
	else
	{
		pid = fork();
		if (pid == 0)
		{
			for (int i = 0; i < NH; i++)
			{
				pid = fork();
				if (pid == 0)
				{
					// Will change shared variables, use semaphore
					sem_wait(semInit);
					// Initialize idO of this process
					int idH = *hydroCurID;
					bool printedCreating = false;
					fprintf(fOutPtr, "%d: H %d: started\n", *actionN, idH);
					// Increment shared actionN
					(*actionN)++;
					// Increment shared oxyCurID
					(*hydroCurID)++;
					sem_post(semInit);

					// sleep, while sleeping other processes can come into action
					usleep(randInt(0, TI));

					sem_wait(semInit);
					// changing shared queue and status, use semaphore
					fprintf(fOutPtr, "%d: H %d: going to queue\n", *actionN, idH);
					(*actionN)++;
					sem_post(semInit);

					// push into queue
					sem_wait(semQ);
					queuePush(hydroQ, idH);
					sem_post(semQ);

					while (1)
					{
						// signal that hydrogen atom was used in creating molecule, so it 
						// can fall into state "molecule created" and exit 
						if (hydroStatus[idH] == 1)
						{
							sem_wait(semInit);
							fprintf(fOutPtr, "%d: H %d: molecule %d created\n", *actionN, idH, *noM);
							(*actionN)++;
							hydroStatus[idH] = 5;
							sem_post(semInit);
							exit(EXIT_SUCCESS);
						}

						// signal that this hydrogen atom was freed from queue, so it
						// can fall into state "creating molecule"
						if (hydroStatus[idH] == 2 && printedCreating == false)
						{
							sem_wait(semInit);
							fprintf(fOutPtr, "%d: H %d: creating molecule %d\n", *actionN, idH, *noM);
							(*actionN)++;
							printedCreating = true;
							hydroStatus[idH] = 4;
							sem_post(semInit);
						}

						// check if we have enough hydrogen or oxygen to create new molecule
						// if not, this hydrogen atom wont be used and we can exit from this process 
						if ((NH - *hydrosUsed) < 2 || (NO - *oxysUsed) < 1)
						{
							fprintf(fOutPtr, "%d: H %d: not enough O or H\n", *actionN, idH);
							(*actionN)++;
							exit(EXIT_SUCCESS);
						}
					}
				}
				else if (pid == -1)
				{
					fprintf(stderr, "ERROR: Couldn't fork\n");
					exit(EXIT_FAILURE);
				}
			}
			// process that creates hydrogen atoms is waiting for it's 
			// children to end
			while (wait(NULL) > 0)
				;
			exit(EXIT_SUCCESS);
		}
		else if (pid == -1)
		{
			fprintf(stderr, "ERROR: Couldn't fork\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			// main process wait for all children to end
			while (wait(NULL) > 0);
			// if all children has terminated, 
			// free all allocated recourses 
			munmap(oxyStatus, sizeof(int) * (NO + 1));
			munmap(hydroStatus, sizeof(int) * (NH + 1));
			munmap(actionN, sizeof(int));
			munmap(noM, sizeof(int));
			munmap(oxyCurID, sizeof(int));
			munmap(hydroCurID, sizeof(int));
			munmap(molBeingCreat, sizeof(int));
			munmap(hydrosUsed, sizeof(int));
			munmap(oxysUsed, sizeof(int));
			munmap(poppedH1, sizeof(int));
			munmap(poppedH2, sizeof(int));
			munmap(poppedO, sizeof(int));
			munmap(oxyQ, sizeof(int) * NO);
			munmap(hydroQ, sizeof(int) * NH);
			sem_close(semInit);
			sem_destroy(semInit);
			sem_close(semCreating);
			sem_destroy(semCreating);
			sem_close(semMol);
			sem_destroy(semMol);
			sem_close(semQ);
			sem_destroy(semQ);
			fclose(fOutPtr);

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
		fprintf(stderr, "ERROR: Invalid number of arguments\n");
		exit(EXIT_FAILURE);
	}

	if (!isDigit(argv[1]) || !isDigit(argv[2]) || !isDigit(argv[3]) || !isDigit(argv[4]))
	{
		fprintf(stderr, "ERROR: All arguments must be numbers!\n");
		exit(EXIT_FAILURE);
	}

	int NO = atoi(argv[1]);
	if (NO < 0)
	{
		fprintf(stderr, "ERROR: Invalid number of oxygen\n");
		exit(EXIT_FAILURE);
	}

	int NH = atoi(argv[2]);
	if (NH < 0)
	{
		fprintf(stderr, "ERROR: Invalid number of hydrogen\n");
		exit(EXIT_FAILURE);
	}

	int TI = atoi(argv[3]);
	if (TI < 0 || TI > 1000)
	{
		fprintf(stderr, "ERROR: Invalid maximum idle time\n");
		exit(1);
	}

	int TB = atoi(argv[4]);
	if (TB < 0 || TB > 1000)
	{
		fprintf(stderr, "ERROR: Invalid maximum time required for molecule creating\n");
		exit(1);
	}
}

/* 
  Queue constructor function. Sets length to 0.
  And sets up pointer to data. Space for data is preallocated
  in shared memory.
*/
void queueCtr(queue *q)
{
	q->len = 0;
	q->data = (int *)q + sizeof(int);
}
/* 
	Queue push, increase length and
	put an element. Space for data is preallocated in shared memory.
*/
void queuePush(queue *q, int element)
{
	q->len++;

	for (int i = q->len; i > 0; i--)
		q->data[i] = q->data[i - 1];

	q->data[0] = element;
}

/*
	Pop an element from the end of the queue and return its value.
	if len is lesser than 0, return -1.
*/
int queuePop(queue *q)
{
	if (q->len <= 0)
		return -1;
	int popped = q->data[q->len - 1];
	q->len--;
	return popped;
}

// The end of the file proj2.c