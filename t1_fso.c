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
    char nome[20];
    int estado;         // 0 == Pronto, 1 == Terminou de executar
    pid_t donoProcesso; // dono == PID, sem dono == -1
    struct no *prox;
    int idAreaMemoria;
} processo;

void insereProcesso(processo **lista, int id, char *str) {
    int no = shmget(IPC_PRIVATE, sizeof(processo), SHM_R | SHM_W | IPC_CREAT);    
    processo *novoProcesso;
    novoProcesso = (processo *) shmat(no, NULL, 0);

    if(novoProcesso) {
        
        novoProcesso->id = id;
        strcpy(novoProcesso->nome, str);

        novoProcesso->estado = 0;
        novoProcesso->donoProcesso = -1;
        novoProcesso->prox = NULL;
        novoProcesso->idAreaMemoria = no;

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

void desmapMemoria(processo **lista) {
    // int i = 0;
    // while (*lista) {
    //     processo *aux = (*lista)->prox;         // Salva o endereco do proximo NÓ
    //     int idMemory = (*lista)->idAreaMemoria; // Salva o Id da memoria compartilhada
    //     shmdt(*lista);                          // Desmapeia/Desanexa o NÓ atual
    //     shmctl(idMemory, IPC_RMID, NULL);       // Destroi a área de memória compartilhada criada
    //     *lista = aux;                           // lista recebe o endereço do NÓ seguinte salvo em aux, virando o NÓ atual
    //                                             // Avança o loop
    //     i++;                                    // Contador
    // }
    // // printf("Nós desanexados: %d\n", i);
    // processo *aux = *lista;
    // while (aux) {
    //     processo *aux2 = aux->prox;
    //     int idMemory = aux->idAreaMemoria;
    //     shmdt(aux);
    //     shmctl(idMemory, IPC_RMID, NULL);
    //     aux = aux2;
    // }
    
    
}

// void atachMemoria(processo **lista) {
//     processo *loop = *lista;
//     while (loop) {
//         processo *aux = loop->prox;
//         int idMemory = loop->idAreaMemoria;
//         *loop = (processo * ) shmat(idMemory, NULL, 0);
//         loop = aux;
//     }
// }

int main() {

    processo *listaProcessosHead = NULL;
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
            insereProcesso(&listaProcessosHead, id, read);
            id++;
        }
        fclose(arqProcessos);
        // printf("id: %d\n", id);

        desmapMemoria(&listaProcessosHead);
        //printf("head id: %d\n" , (*listaProcessosHead).id);
        printProcessos(listaProcessosHead);


        
        // pid_t p_auxs[2];
        // int status0 = -1;
        // int status1 = -1;

        // int mA = shmget(IPC_PRIVATE, sizeof(int) * 3, SHM_R | SHM_W | IPC_CREAT);
        // int *a;
        // a = (int *) shmat(mA, NULL, 0);

        // *a = 42;
        

        

        // for(int i=0; i<2; i++) {
        //     p_auxs[i] = fork();
            

        //     if (p_auxs[i] == 0) {
        //         //printf("FILHO(%d), meu pai é: %d\n", getpid(), getppid());
                
                
        //         switch (i) {
        //             case 0:
        //                     printf("Sou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());
                            
                            

        //                     //atachMemoria(&listaProcessosHead);
                            
        //                     processo *aux = listaProcessosHead;
        //                     while (aux) {
        //                         aux = (processo *) shmat(aux->idAreaMemoria, NULL, 0);
        //                         aux = aux->prox;
        //                     }
                            
        //                     printProcessos(listaProcessosHead);


                            
        //                     a = (int *) shmat(mA, NULL, 0);
        //                     printf("A: %d --\n", *a);
        //                     *a = 54;
        //                     //desmapMemoria(&listaProcessosHead);
        //                     //printProcessos(listaProcessosHead);
        //                     strcpy(buscaProcesso(&listaProcessosHead, -1)->nome, "LUCAS");
        //                     printProcessos(listaProcessosHead);
        //                     // processo *retorno = buscaProcesso(&listaProcessos, -1);
        //                     // if(retorno->id == 0) {
        //                     //     retorno->id = 20;
        //                     //     printf("Achou processo: %d\n", retorno->id);
        //                     // }

                            
        //                     printf("\nPassou...\n");
        //                     //shmdt(listaProcessos);

        //                     _exit(i);
                            
        //                     break;
                    
        //             case 1:
        //                     //listaProcessos = (processo *) shmat(memoriaCompartilhada, NULL, 0);
        //                     printf("Sou filho(%d) pid:%d    PAI(%d)\n", i, getpid(), getppid());
        //                     sleep(20);
        //                     printf("A1: %d\n", *a);
        //                     *a = 35;
        //                     //printProcessos(listaProcessos);

        //                     //shmdt(listaProcessos);

        //                     _exit(i);
        //                     break;
        //             default:
        //                     break;
        //         }
        //     }

        // }

        // printf("PAI(%d), meu filho são: %d e %d\n\n", getpid(), p_auxs[0], p_auxs[1]);
        

        // for(int i=0; i<2; i++) {
        //     switch (i) {
        //             case 0:
        //                     wait(&status0);
        //                     if(WIFEXITED(status0)) {
        //                         printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status0), p_auxs[i]);
        //                     }
        //                     break;

        //             case 1:
        //                     wait(&status1);
        //                     if(WIFEXITED(status1)) {
        //                         printf("Meu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status1), p_auxs[i]);
        //                     }
        //                     break;
        //             default:
        //                     break;
        //     }

            
        // }

        // printf("A1: %d\n", *a);
        //printProcessos(listaProcessosHead);
        //desmapMemoria(&listaProcessosHead);

        // shmdt(listaProcessosHead);
        // shmctl(headNo, IPC_RMID, NULL);
    }

    return 0;
}