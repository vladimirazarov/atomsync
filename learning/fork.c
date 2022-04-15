#include <stdio.h>
#include <unistd.h>

int main()
{
    int pid = fork();
    if (pid == 0)
    {
        printf("Im your child bro, my pid is %d\n", getpid());
    }
    else if (pid == 1)
    {
        printf("Cant make children, go to a doctor %d\n");
    }
    else
    {
        printf("Im parent bro, my pid is %d\n", getpid());
    }
}