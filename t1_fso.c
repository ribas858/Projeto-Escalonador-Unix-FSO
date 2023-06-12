#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/wait.h>

#include<unistd.h>

#include<sys/ipc.h>
#include<sys/shm.h>

#define PROCESSOS_AUX 4

typedef struct no {
    int id;
    char nome[20];
    int estado;         // 0 == Pronto, 1 == Terminou de executar
    pid_t donoProcesso; // dono == PID, sem dono == -1
    struct no *prox;
} processo;

void insereProcesso(processo **lista, int id, char *str) {  
    processo *novoProcesso = malloc(sizeof(processo));

    if(novoProcesso) {
        
        novoProcesso->id = id;
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

processo* buscaProcesso(processo **lista, pid_t pid) {
    int achou = 0;
    processo *aux = malloc(sizeof(processo));
    aux = *lista;
    while(aux->prox) {
        if(aux->donoProcesso == pid) {
            achou = 1;
            break;
        }
        aux = aux->prox;
    }

    if(achou == 1) {
        return aux;
    }
    return NULL;
}

void listaParaListaCompartilhada(processo *lista, processo *listaCompart) {
    if(listaCompart == NULL) {
        printf("\nMemória compartilha não existe...\n\n");
    } else {
        processo *auxLista = listaCompart;
        while(lista) {
            auxLista->id = lista->id;
            strcpy(auxLista->nome, lista->nome);
            auxLista->estado = lista->estado;
            auxLista->donoProcesso = lista->donoProcesso;
            
            if(lista->prox == NULL) {
                auxLista->prox = NULL;    
            } else {
                auxLista->prox = auxLista + 1;
            }

            auxLista = auxLista + 1;
            
            //printf("id:%d\n", lista->id);
            lista = lista->prox;
        }
    }
}

void liberaListaProcessos(processo **lista) {
    while(*lista) {
        processo *aux = (*lista)->prox;
        free(*lista);
        *lista = aux;
    }
}

int main() {

    processo *listaProcessos = NULL;
    processo *sharedListProcessos;
    char *read = malloc(sizeof(char) * 20);
    FILE *arqProcessos;
    
    arqProcessos = fopen("processos.txt", "r");
    
    if (arqProcessos == NULL) {
        printf("ERRO! O arquivo de processos não foi aberto!\n");

    } else {

        int id = 0;
        while(fgets(read, 20, arqProcessos) != NULL) {
            int tam = strlen(read);
            read[tam-1] = '\0';
            insereProcesso(&listaProcessos, id, read);
            id++;
        }
        fclose(arqProcessos);
        printf("Processos: %d\n", id);

        int idMemoryCompart = shmget (IPC_PRIVATE, sizeof(processo) * id, SHM_R | SHM_W | IPC_CREAT);
        sharedListProcessos = (processo *) shmat(idMemoryCompart, NULL, 0);

        int memo = shmget(IPC_PRIVATE, sizeof(int) * 3, SHM_R | SHM_W | IPC_CREAT);
        int *striped;
        striped = (int *) shmat(memo, NULL, 0);
        striped[0] = -1;    // Flag para finalizar a distribuicao dos processos
        striped[1] = 0;     // Valor de alternancia para garantir a ordem correta na distribuicao
        striped[2] = 0;     // Contador para percorrer a lista de processos
        

        listaParaListaCompartilhada(listaProcessos, sharedListProcessos);
        liberaListaProcessos(&listaProcessos);

        pid_t p_auxs[PROCESSOS_AUX];
        int status[PROCESSOS_AUX];
        
        //printf("\n================= Codigo DO pai...=================\n");
        for(int i=0; i<PROCESSOS_AUX; i++) {
            p_auxs[i] = fork();
            //printf("\n================= FORK() %d =================\n", i);
            if (p_auxs[i] == 0) {
                
                switch (i) {
                    case 0:
                            striped = (int *) shmat(memo, NULL, 0);
                            sharedListProcessos = (processo *) shmat(idMemoryCompart, NULL, 0);

                            while (striped[0] == -1) {
                                printf("\nSou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());

                                if (striped[2] >= id - 1) {
                                    striped[0] = 1;
                                    printf("FIM============STRIPED..\n");
                                }

                                //printf("Entrou strip 0..\n");
                                while(striped[1] != 0);
                                //printf("Passou beasy wait 0..\n");
                                
                                processo *aux = sharedListProcessos;
                                if (striped[2] < id) {
                                    aux = aux + striped[2];
                                    aux->donoProcesso = getpid();
                                    striped[2]++;
                                }
                    
                                striped[1] = 1;

                            }
                            
                            
                            printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                            
                            
                            pid_t execPid = fork();
                            if(execPid == 0) {
                                execl("processos/lento", "medio", (char * ) NULL);
                            }
                            else if(execPid < 0) {
                                printf("Erro no fork() para o execl\n");
                            }

                            shmdt(striped);
                            shmdt(sharedListProcessos);

                            exit(i);
                            break;
                    
                    case 1:
                            striped = (int *) shmat(memo, NULL, 0);
                            sharedListProcessos = (processo *) shmat(idMemoryCompart, NULL, 0);

                            while (striped[0] == -1) {
                                printf("\nSou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());

                                if (striped[2] >= id - 1) {
                                    striped[0] = 1;
                                }

                                //printf("Entrou strip 1.. %d\n", striped[1]);
                                while(striped[1] != 1);
                                //printf("Passou beasy wait 1..\n");
                                
                                processo *aux = sharedListProcessos;
                                if (striped[2] < id) {
                                    aux = aux + striped[2];
                                    aux->donoProcesso = getpid();
                                    striped[2]++;
                                }
                            
                                striped[1] = 2;
                            }

                            printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);

                            shmdt(striped);
                            shmdt(sharedListProcessos);

                            exit(i);
                            break;
                    
                    case 2:
                            striped = (int *) shmat(memo, NULL, 0);
                            sharedListProcessos = (processo *) shmat(idMemoryCompart, NULL, 0);

                            while (striped[0] == -1) {
                                printf("\nSou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());

                                if (striped[2] >= id - 1) {
                                    striped[0] = 1;
                                }

                                //printf("Entrou strip 2.. %d\n", striped[1]);
                                while(striped[1] != 2);
                                //printf("Passou beasy wait 2..\n");
                                
                                processo *aux = sharedListProcessos;
                                if (striped[2] < id) {
                                    aux = aux + striped[2];
                                    aux->donoProcesso = getpid();
                                    striped[2]++;
                                }
                            
                                striped[1] = 3;
                            }

                            printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);

                            shmdt(striped);
                            shmdt(sharedListProcessos);

                            _exit(i);
                            break;
                    
                    case 3:
                            striped = (int *) shmat(memo, NULL, 0);
                            sharedListProcessos = (processo *) shmat(idMemoryCompart, NULL, 0);

                            while (striped[0] == -1) {
                                printf("\nSou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());

                                if (striped[2] >= id - 1) {
                                    striped[0] = 1;
                                }

                                //printf("Entrou strip 3.. %d\n", striped[1]);
                                while(striped[1] != 3);
                                //printf("Passou beasy wait 3..\n");
                                
                                processo *aux = sharedListProcessos;
                                if (striped[2] < id) {
                                    aux = aux + striped[2];
                                    aux->donoProcesso = getpid();
                                    striped[2]++;
                                }
                            
                                striped[1] = 0;
                            }

                            printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);

                            shmdt(striped);
                            shmdt(sharedListProcessos);

                            _exit(i);
                            break;
                    
                    default:
                            break;
                }
            }
            

        }
        // printf("\n================= FIM DO For de Forks =================\n");

        printf("\nPAI(%d), meus filhos são: ", getpid());
        for (int i=0; i<PROCESSOS_AUX; i++) {
            printf("%d e ", p_auxs[i]);
        }
        printf("\n\n");
        
        

        for(int i=0; i<PROCESSOS_AUX; i++) {
            switch (i) {
                    case 0:
                            wait(&status[i]);
                            if(WIFEXITED(status[i])) {
                                printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status[i]), p_auxs[i]);
                            }
                            break;

                    case 1:
                            wait(&status[i]);
                            if(WIFEXITED(status[i])) {
                                printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status[i]), p_auxs[i]);
                            }
                            break;

                    case 2:
                            wait(&status[i]);
                            if(WIFEXITED(status[i])) {
                                printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status[i]), p_auxs[i]);
                            }
                            break;
                    
                    case 3:
                            wait(&status[i]);
                            if(WIFEXITED(status[i])) {
                                printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status[i]), p_auxs[i]);
                            }
                            break;
                            
                    default:
                            break;
            }

            
        }

        printProcessos(sharedListProcessos);

        shmdt(striped);
        shmdt(sharedListProcessos);

        shmctl(memo, IPC_RMID, NULL);
        shmctl(idMemoryCompart, IPC_RMID, NULL);
    }

    return 0;
}