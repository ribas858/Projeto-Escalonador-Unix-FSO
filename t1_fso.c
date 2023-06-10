#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/wait.h>

#include<unistd.h>

#include<sys/ipc.h>
#include<sys/shm.h>


typedef struct no {
    int id;
    char *nome;
    int estado;         // 0 == Pronto, 1 == Terminou de executar
    pid_t donoProcesso; // dono == PID, sem dono == -1
    struct no *prox;
} processo;

void insereProcesso(processo **lista, int id, char *str) {
    processo *novoProcesso = malloc(sizeof(processo));
    if(novoProcesso) {
        novoProcesso->id = id;

        novoProcesso->nome = malloc(sizeof(char) * 20);
        strcpy(novoProcesso->nome, str);

        novoProcesso->estado = 0;
        novoProcesso->donoProcesso = -1;
        novoProcesso->prox = NULL;

        if (*lista == NULL){
            *lista = novoProcesso;
        } else {
            processo *aux = malloc(sizeof(processo));
            aux = *lista;
            while (aux->prox) {
                aux = aux->prox;
            }
            aux->prox = novoProcesso;
        }

    } else {
        printf("Erro ao inserir processo\n");
    }
}

void printProcessos(processo *lista) {
    while (lista) {
        printf("\nId: %d\n", lista->id);
        printf("Nome: %s\n", lista->nome);
        printf("Estado: %d\n", lista->estado);
        printf("Dono Processo: %d\n", lista->donoProcesso);
        
        lista = lista->prox;
    }
}

int main() {

    char *read = malloc(sizeof(char) * 20);
    char nome[20];

    FILE *arqProcessos;
    processo *listaProcessos = NULL;
    
    arqProcessos = fopen("processos.txt", "r");
    
    if (arqProcessos == NULL) {
        printf("ERRO! O arquivo de processos não foi aberto!\n");

    } else {

        int i = 0;
        while(fgets(read, 20, arqProcessos) != NULL) {
            int tam = strlen(read);
            read[tam-1] = '\0';
            insereProcesso(&listaProcessos, i, read);
            i++;
        }
        fclose(arqProcessos);

        printProcessos(listaProcessos);

        
        
        
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
    }

    return 0;
}