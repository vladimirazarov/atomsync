#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define PAGESIZE 4096

int v = 5; 

int main()
{
    uint8_t *shared_memory = mmap(NULL, PAGESIZE,
                                  PROT_READ | PROT_WRITE, 
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (fork()==0){
        *shared_memory = 15;
        v = 80;
    }
    else {
        wait(NULL);
    }
    printf("Not shared. %i\n", v);
    printf("Shared. %i\n", *shared_memory);
}