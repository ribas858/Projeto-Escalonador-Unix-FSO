#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/wait.h>

#include<unistd.h>

#include<sys/ipc.h>
#include<sys/shm.h>


int main() {

    pid_t p_auxs[2];
    int status0 = -1;
    int status1 = -1;
    

    int memoA = shmget(IPC_PRIVATE, sizeof(int), SHM_R | SHM_W | IPC_CREAT);

    int *a = (int *) shmat(memoA, NULL, 0);

    *a = 1;

    for(int i=0; i<2; i++) {
        p_auxs[i] = fork();
        

        if (p_auxs[i] == 0) {
            //printf("FILHO(%d), meu pai é: %d\n", getpid(), getppid());

            a = (int *) shmat(memoA, NULL, 0);
            
            
            switch (i) {
                case 0:
                        
                        printf("A: %d || Sou filho(%d) pid:%d    PAI(%d)\n", *a,  i, getpid(), getppid());
                        
                        
                        while (*a != 50);
                        
                        printf("\nPassou...\n");
                        shmdt(a);
                        _exit(i);
                        
                        break;
                
                case 1:
                        
                        printf("B: %d || Sou filho(%d) pid:%d    PAI(%d)\n", *a,  i, getpid(), getppid());
                        sleep(20);
                        *a = 50;
                        shmdt(a);
                        _exit(i);
                        break;
                default:
                        break;
            }
        }

    }

    printf("PAI(%d), meu filho são: %d e %d\n\n", getpid(), p_auxs[0], p_auxs[1]);
    

    for(int i=0; i<2; i++) {
        switch (i) {
                case 0:
                        wait(&status0);
                        if(WIFEXITED(status0)) {
                            printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status0), p_auxs[i]);
                        }
                        break;

                case 1:
                        wait(&status1);
                        if(WIFEXITED(status1)) {
                            printf("Meu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status1), p_auxs[i]);
                        }
                        break;
                default:
                        break;
        }

        
    }

    shmdt(a);
    shmctl(memoA, IPC_RMID, NULL);

    return 0;
}