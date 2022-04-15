#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>

#define IPC_RES_ERR (-1)

// bytes
#define BLOCK_SIZE 4096 
#define FILENAME "shMemBlocksOps.c" 

int getSharedBlock(char *fileName, int size)
{
    key_t key;
    // request a key that will be linked to a filename
    key = ftok(fileName, 0);
    if (key == IPC_RES_ERR)
    {
        return IPC_RES_ERR;
    }
    return shmget(key, size, 0644 | IPC_CREAT); // valid block ID or -1
}

char *attachMemBlock(char *fileName, int size)
{
    int sharedBlkID = getSharedBlock(fileName, size);
    char *result;

    if (sharedBlkID == IPC_RES_ERR)
        return NULL;

    // map the shared block int this process's memory
    // and give me a pointer to it 
    result = shmat(sharedBlkID, NULL, 0);

    if (result == (char *)IPC_RES_ERR)
        return NULL;
}

bool detachMemBlk(char *block)
{
    return (shmdt(block) != IPC_RES_ERR);
}

bool dstrMemBlk(char *filename)
{
    int sharedBlkID = getSharedBlock(filename, 0);

    if (sharedBlkID == IPC_RES_ERR)
        return NULL;

    return (shmctl(sharedBlkID, IPC_RMID, NULL) != IPC_RES_ERR);
}

int main(int argc, char **argv)
{
    //grab the sh mem blk
    char *block = attachMemBlock(FILENAME, BLOCK_SIZE);
    if (block == NULL)
    {
        fprintf(stderr, "Couldnt get block\n");
        return -1;
    }

    printf("Writing: \"%s\"\n", argv[1]);
    strncpy(block, argv[1], BLOCK_SIZE);
    printf("Reading: \"%s\"\n", block);

    
    detachMemBlk(block);

    if (dstrMemBlk(FILENAME))
        printf("Destroyed block: %s\n", FILENAME);
    else 
        printf("Could not destroy block: %s\n", FILENAME);

    return 0;
}
