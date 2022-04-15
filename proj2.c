#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>


// Functions headers
void validateInput(int argc, char **argv);
int isDigit(char *str);

// Global variables
sem_t *sem = NULL;


int main(int argc, char **argv)
{
	// argv[0] ... program
	// argv[1] ... oxygen number ... int NO ... >=0
	// argv[2] ... hydrogen number ... int NH ... >=0
	// argv[3] ... max time to wait before hydrogen or oxygen will go to the queue ... int TI ... 0<=TI<=1000
	// argv[4] ... max time for creating 1 molecule ... int TB ... 0<=TB<=1000
	validateInput(argc, argv);

}

/* 
   Return 1 if string is full of digits.
   Otherwise return 0 if at least one letter 
   or other non-digit character is met 
*/
int isDigit(char *str) {
  int i;
  for (i = 0; str[i] != '\0'; i++) {
    if (str[i] < '0' || str[i] > '9') {
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