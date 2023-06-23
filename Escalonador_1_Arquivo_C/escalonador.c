#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

#include<time.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/wait.h>
#include<sys/types.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// VARIÁVEIS E ESTRUTURAS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PROCESSOS_AUX 4                                                         // Máximo 4 processos auxiliares

// Estrutura para processos
typedef struct no {
    int id;
    int estado;                                                                 // 0 == Pronto, 1 == Terminou de executar, 3 == Terminou de executar (Roubado)
    char nome[20];
    struct no *prox;
    pid_t donoProcesso;                                                         // dono == PID, sem dono == -1
} processo;

// Estrutura para tabela de dados
typedef struct est {
    int id;
    int pid;
    int exec;                                                                   // 1 == meu processo, 2 == roubado
    int tempo;                                                                  // Tempo de execução individual
    int status;                                                                 // 1 == Terminou de executar
    int ocupado;                                                                // 1 == ocupado, 0 disponivel
    char nome[20];
    int idProcessoExec;
    int idDonoOriginal;
} estatistica;

struct sembuf operacao[2];                                                      // Estrutura de operações com Semaforos

int idSemaforo;                                                                 // Id do semaforo
int numProcessos;                                                               // Numero total de processos a serem executados
int idMemoriaEst;                                                               // Id da memoria compartilhada 'listaEst'
int idMemoriaExecutou;                                                          // Id da memoria compartilhada 'executou'
int idMemoriaProcessos;                                                         // Id da memoria compartilhada 'sharedListProcessos'
int idMemoriaStripedVetor;                                                      // Id da memoria compartilhada 'stripedFlag'

int *executou;                                                                  // Variavel auxiliar no EXECL()
int *stripedFlag;                                                               // Vetor de flags para distribuição striped
estatistica *listaEst;                                                          // Lista de dados de execução
processo *sharedListProcessos;                                                  // Lista encadeada de processos, no segmento de memoria compartilhada

key_t semKey = 0x00000862;                                                      // Chave para semaforo
key_t memo1Key = 0x00000858;                                                    // Chave 1 para segmento de memoria compartilhada
key_t memo2Key = 0x00000859;                                                    // Chave 2 para segmento de memoria compartilhada
key_t memo3Key = 0x00000860;                                                    // Chave 3 para segmento de memoria compartilhada

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// PROTÓTIPOS DE FUNÇÕES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int p_sem();                                                                    // Semáforo: Pega permissão    
int v_sem();                                                                    // Semáforo: Devolve permissão
void limpeza();                                                                 // Responsável por desalocar o semaforo, e os segmentos de memória
                                                                                // compartilhada. Tanto no fim do programa, ou no caso de receber um sinal.

void alocaIPCs();                                                               // Responsável por alocar os segmentos de memória compartilhada e o semáforo.
void limpezaExec();                                                             // Responsável por desalocar um segmento de memória compartilhada usada
                                                                                // no auxilio da execução dos processos

void execRoubo(int *tempo);                                                     // Responsável por executar o escalonamento no modo ROUBO
void execNormal(int *tempo);                                                    // Responsável por executar o escalonamento no modo NORMAL
void minusculas(char *read);                                                    // Responsável por passar qualquer caracter lido do nome dos
                                                                                // processos para minusculo, e remover espaços em branco e '\n'

void printProcessos(processo *lista);                                           // Responsável por printar a lista encadeada de processos atribuidos
void liberaListaProcessos(processo **lista);                                    // Responsável por liberar a memoria da lista encadeada, após os dados
                                                                                // serem copiados para o segmento de memória compartilhada

int meusProcessos(processo *lista, pid_t pid);                                  // Responsável por retornar o numero de processos atribuidos a um processo AUX
void executaAUX(int *tempo, int *modo, int *id);                                // Responsável executar o codigo dos processos Auxiliares
int leArquivo(processo **lista, char *nomeArquivo);                             // Responsável por ler o arquivo e inserir os processos na lista encadeada
void updateEstatistica(int id, int tempo, int status);                          // Responsável por atulizar o tempo e o estado de um processo na lista de dados
void printEstatistica(estatistica *lista, int *p_auxs);                         // Responsável por printar a tabela de dados
void testeArgumentos(int argc, char *argv[], int *modo);                        // Responsável por testar a entrada na linha de comando, afim de evitar erros
void insereProcesso(processo **lista, int id, char *str);                       // Responsável por inserir um processo na lista encadeada
processo* buscaProcesso(processo *lista, pid_t pid, int modo);                  // Responsável por retornar um processo, seja para o modo '1' (retorna para o dono e com estado pronto), ou, para o modo '2' (retorna o primeiro com estado pronto)
int atribuiProcesso(int striped_dois, processo *sharedListProcessos);           // Responsável por definir o dono do processo
void listaParaListaCompartilhada(processo *lista, processo *listaCompart);      // Responsável por copiar a lista encadeana na memoria, para outra lista encadeada no segmento de memoria compartilhada
void insereEstatistica(int pidExecutor, processo *aux, int pid, int exec);      // Responsável por inserir os dados de execução de um processo na tabela de dados
void striped(int *stripedFlag, processo *sharedListProcessos, int processoAux); // Responsável pela distribuição modo Striped

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// IMPLEMENTAÇÃO DAS FUNÇÕES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Semáforo: Pega permissão
int p_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = 0;
    operacao[0].sem_flg = 0;
    operacao[1].sem_num = 0;
    operacao[1].sem_op = 1;
    operacao[1].sem_flg = 0;

    if (semop(idSemaforo, operacao, 2) < 0) {
        printf("Erro PSEM no P=%d\n", errno);
    }
}

// Semáforo: Devolve permissão
int v_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = -1;
    operacao[0].sem_flg = 0;
    if (semop(idSemaforo, operacao, 1) < 0) {
        printf("Erro VSEM no P=%d\n", errno);
    }
}

// Responsável por desalocar o semaforo, e os segmentos de memória compartilhada. Tanto no fim do programa, ou no caso de receber um sinal.
void limpeza() {
    printf("\nESCALONADOR(): Liberando memória.....\n");

    if (semctl(idSemaforo, 0, IPC_RMID, operacao) < 0) {
        printf("Erro ao destruir Semaforos...\n");
    }
    if(shmdt(stripedFlag) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmdt(sharedListProcessos) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmdt(listaEst) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaStripedVetor, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaProcessos, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaEst, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }

    exit(0);
}

// Responsável por alocar os segmentos de memória compartilhada e o semáforo.
void alocaIPCs() {
    idSemaforo = semget(semKey, 1, IPC_CREAT | 0x1ff);
    if (idSemaforo < 0) {
        printf("ERRO! Semaforo não foi criado!\n");
        exit(1);
    }

    idMemoriaProcessos = shmget (memo1Key, sizeof(processo) * numProcessos, 0x1ff | IPC_CREAT);
    if (idMemoriaProcessos < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaProcessos' não foi criado!\n");
        exit(1);
    }
    sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);

    idMemoriaStripedVetor = shmget(memo2Key, sizeof(int) * 4,  0x1ff | IPC_CREAT);
    if (idMemoriaStripedVetor < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaStripedVetor' não foi criado!\n");
        exit(1);
    }
    stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);

    idMemoriaEst = shmget (memo3Key, sizeof(estatistica) * numProcessos, 0x1ff | IPC_CREAT);
    if (idMemoriaEst < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaEst' não foi criado!\n");
        exit(1);
    }
    listaEst = (estatistica *) shmat(idMemoriaEst, NULL, 0);
}

// Responsável por desalocar um segmento de memória compartilhada usada no auxilio da execução dos processos
void limpezaExec() {
    printf("\nLiberando memória.....EXEC\n");
    if(shmdt(executou) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada 'executou'...\n");
    }
    if(shmctl(idMemoriaExecutou, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada  'idMemoriaExecutou'...\n");
    }
    exit(0);
}

// Responsável por executar o escalonamento no modo ROUBO
void execRoubo(int *tempo) {
    processo *aux = buscaProcesso(sharedListProcessos, 0, 2);                   // Devolve o primeiro processo pronto(estado 0), independente do dono do processo
        while (aux) {
            printf("Processo AUX(%d): Roubei um processo - Dono: AUX(%d) - id: %d - estado: %d\n\n", getpid(), aux->donoProcesso, aux->id, aux->estado);
            
            p_sem();
            aux->estado = 2;                                                    // Passa para o estado Executando, para evitar interrupções
            v_sem();
            
            char path[50] = {"processos/"};
            strcat(path, aux->nome);                                            // Monta o caminho para o processo a ser executado

            idMemoriaExecutou = shmget (IPC_PRIVATE, sizeof(int), 0x1ff | IPC_CREAT);
            executou = (int *) shmat(idMemoriaExecutou, NULL, 0);               // Anexa segmento de memoria compartilhada a variavel 'executou'
            *executou = 1;                                                      // Variavel flag, responsável por indicar se o EXECL() foi bem sucedido
                                                                                // Caso permaneça com valor 1, ocorreu tudo bem
            
            signal(SIGTERM, limpezaExec);                                       // Sinais que aguardam caso seja necessário finalizar o programa por interrupções
            signal(SIGINT, limpezaExec);                                        // Caso aconteça, os segmentos e semaforos são removidos da memoria.
            signal(SIGSEGV, limpezaExec);                                       //
            

            time_t begin = time(NULL);                                          // Marca do tempo de inicio de execução do processo
            pid_t execPid = fork();                                             // Fork() para ser substituido pelo programa alvo do EXECL()

            if(execPid == 0) {
                                                                                // Código do processo filho do Processo AUX
                p_sem();
                insereEstatistica(getppid(), aux, getpid(), 2);                 // Inserção de dados para relatar ao fim da execução
                v_sem();

                execl(path, aux->nome, (char *) NULL);                          // Executar o programa(processo atribuido) indicado no caminho 'path'

                executou = (int *) shmat(idMemoriaExecutou, NULL, 0);           // Se chegar aqui, o EXECL() deu erro
                *executou = 0;                                                  // Então a variavel 'executou se torna zero'

                shmdt(executou);                                                // Desanexa 'executou' da memoria compartilhada
                exit(0);                                                        // Finaliza o processo neto
            }

            wait(NULL);                                                         // Aguarda até que o processo atribuido, ou o processo neto(no caso de erro) termine
            time_t end = time(NULL);                                            // Marca o fim do tempo de execução do processo atribuido
            
            if (*executou == 1) {                                               // Se o valor de 'executou' continua 1, ocorreu tudo bem com o EXECL()
            
                p_sem();                                                        // Passa para o estado "Executado especial", ou seja, o processo atribuido foi
                aux->estado = 3;                                                // executado por um processo auxiliar que não era o seu dono.
                v_sem();

                *tempo = end - begin;                                           // Calcula o tempo de execução

                
                p_sem();                                                        // Atualiza o tempo, e o estado de execução na tabela de dados
                updateEstatistica(aux->id, *tempo, 1);                          // O estado 1 esta sendo passado para definir que o programa foi executado
                v_sem();                                                        // normalmente, mas a variavel 'estado' do processo atribuido continua com o valor 3,
                                                                                // e ainda sera atualizada pelo proprio dono quando ele perceber que o processo
                                                                                // foi roubado
                
                printf("Processo AUX(%d): --->>>>> FIM EXEC - ROUBO '%s' Id(%d) Tempo %d Segundos\n\n", getpid(), aux->nome, aux->id, *tempo);
            } else {
                printf("----------------->>>>> ERRO! Processo '%s' Id(%d) NÃO EXECUTOU\n\n", aux->nome, aux->id);
                aux->estado = -1;                                               // Estado de erro, processo atribuido não foi executado
            }

            shmdt(executou);                                                    // Desanexa 'executou' da memoria compartilhada
            shmctl(idMemoriaExecutou, IPC_RMID, NULL);                          // Deleta segmento de memoria 'idMemoriaExecutou'
            printf("Processo AUX(%d): Liberando memória....EXEC\n\n", getpid());

            aux = buscaProcesso(sharedListProcessos, 0, 2);                     // Busca o próximo processo (estado 0) disponivel para avançar o loop while
        }
        printf("Processo AUX(%d): Sem mais processos para executar, adeus...\n\n", getpid());
}

// Responsável por executar o escalonamento no modo NORMAL
void execNormal(int *tempo) {

    p_sem();
    int qtdProcessos = meusProcessos(sharedListProcessos, getpid());            // Retorna a quantidade de processos atribuidos ao processo AUX
    v_sem();

    while (qtdProcessos > 0) {                                                  // Enquanto não acabar seus processos, ele executa

        p_sem();
        processo *aux = buscaProcesso(sharedListProcessos, getpid(), 1);        // Devolve o processo atribuido ao seu PID, e com estado 0 (pronto)
        v_sem();

        if(aux) {
            if(aux->estado == 0) {                                              // Se o processo esta pronto

                printf("Processo AUX(%d): Encontrei meu processo..id(%d) -- estado(%d) 'Pronto'\n\n", getpid(), aux->id, aux->estado);
                p_sem();
                aux->estado = 2;                                                // Passa para o estado Executando, para evitar interrupções
                v_sem();
                
                char path[50] = {"processos/"};
                strcat(path, aux->nome);                                        // Monta o caminho para o processo a ser executado

                idMemoriaExecutou = shmget (IPC_PRIVATE, sizeof(int), 0x1ff | IPC_CREAT);
                executou = (int *) shmat(idMemoriaExecutou, NULL, 0);           // Anexa segmento de memoria compartilhada a variavel 'executou'
                *executou = 1;                                                  // Variavel flag, responsável por indicar se o EXECL() foi bem sucedido
                                                                                // Caso permaneça com valor 1, ocorreu tudo bem
                
                signal(SIGTERM, limpezaExec);                                   // Sinais que aguardam caso seja necessário finalizar o programa por interrupções
                signal(SIGINT, limpezaExec);                                    // Caso aconteça, os segmentos e semaforos são removidos da memoria.
                signal(SIGSEGV, limpezaExec);                                   //

                time_t begin = time(NULL);                                      // Marca do tempo de inicio de execução do processo
                pid_t execPid = fork();                                         // Fork() para ser substituido pelo programa alvo do EXECL()

                if(execPid == 0) {                                              
                                                                                // Código do processo filho do Processo AUX
                    p_sem();
                    insereEstatistica(getppid(), aux, getpid(), 1);             // Inserção de dados para relatar ao fim da execução
                    v_sem();

                    execl(path, aux->nome, (char *) NULL);                      // Executar o programa(processo atribuido) indicado no caminho 'path'

                    executou = (int *) shmat(idMemoriaExecutou, NULL, 0);       // Se chegar aqui, o EXECL() deu erro
                    *executou = 0;                                              // Então a variavel 'executou se torna zero'

                    shmdt(executou);                                            // Desanexa 'executou' da memoria compartilhada
                    exit(0);                                                    // Finaliza o processo neto
                }

                wait(NULL);                                                     // Aguarda até que o processo atribuido, ou o processo neto(no caso de erro) termine
                time_t end = time(NULL);                                        // Marca o fim do tempo de execução do processo atribuido

                if (*executou == 1) {                                           // Se o valor de 'executou' continua 1, ocorreu tudo bem com o EXECL()
                    
                    p_sem();
                    aux->estado = 1;                                            // Atribui o estado como '1', ou seja, processo executado e finalizado com sucesso
                    qtdProcessos--;                                             // Marca menos 1 na quantidade de processos para executar.
                    v_sem();

                    *tempo = end - begin;                                       // Calcula o tempo de execução 

                    p_sem();                                                    // Atualiza o tempo, e o estado de execução na tabela de dados
                    updateEstatistica(aux->id, *tempo, aux->estado);            //
                    v_sem();                                                    //

                    printf("Processo AUX(%d): --->>>>> FIM EXEC.... '%s' Id(%d) Tempo %d Segundos\n\n", getpid(), aux->nome, aux->id, *tempo);
                } else {
                    printf("Processo AUX(%d): --->>>>> ERRO! Processo '%s' Id(%d) NÃO EXECUTOU\n\n", getpid(), aux->nome, aux->id);
                    aux->estado = -1;                                           // Estado de erro, processo atribuido não foi executado
                    qtdProcessos--;                                             // Mesmo em casos de erro, a quantidade de processos é diminuida
                                                                                // para evitar a paralização do programa, mas ao final teremos o 
                                                                                // estado de 'erro' em sua execução
                }

                shmdt(executou);                                                // Desanexa 'executou' da memoria compartilhada
                shmctl(idMemoriaExecutou, IPC_RMID, NULL);                      // Deleta segmento de memoria 'idMemoriaExecutou'
                printf("Processo AUX(%d): Liberando memória....EXEC\n\n", getpid());
            }
            else if(aux->estado == 3) {                                         // Caso o processo esteja no estado '3' ROUBADO, ele já foi executado corretamente
                                                                                // por outro processo AUX
                printf("Processo AUX(%d): Processo(%d) já executado por outro AUX. estado(%d) 'Roubado'\n\n", getpid(), aux->id, aux->estado);
                p_sem();
                aux->estado = 1;                                                // Passamos o estado de '3' para '1' para marcar que o processo já foi executado
                qtdProcessos--;                                                 // Marcamos menos 1 na quantidade de processos atribuidos
                v_sem();
            }
        }
    }
}

// Responsável por passar qualquer caracter lido do nome dos processos para minusculo, e remover espaços em branco e '\n'
void minusculas(char *read) {
    int cont = 0;
    int tam = strlen(read);
    for(int i=0; i<tam; i++) {
        if(read[i] != ' ') {
            read[i] = tolower(read[i]);
            cont++;
        }
    }
    read[cont] = '\0';
}

// Responsável por printar a lista encadeada de processos atribuidos
void printProcessos(processo *lista) {
    while (lista) {
        printf("\nId: %d\n", lista->id);
        printf("Nome: %s\n", lista->nome);
        printf("Estado: %d\n", lista->estado);
        printf("Dono Processo: %d\n", lista->donoProcesso);
        lista = lista->prox;
    }
}

// Responsável por liberar a memoria da lista encadeada, após os dados serem copiados para o segmento de memória compartilhada
void liberaListaProcessos(processo **lista) {
    while(*lista) {
        processo *aux = (*lista)->prox;
        free(*lista);
        *lista = aux;
    }
}

// Responsável por retornar o numero de processos atribuidos a um processo AUX
int meusProcessos(processo *lista, pid_t pid) {
    int cont = 0;
    while (lista) {
        if(lista->donoProcesso == pid) {
            cont++;
        }
        lista = lista->prox;
    }
    return cont;
}

// Responsável executar o codigo dos processos Auxiliares
void executaAUX(int *tempo, int *modo, int *id) {
    p_sem();                                                 
    stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
    sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                                                            // Anexa os segmentos de memoria compartilhada
    v_sem();                                                //

    striped(stripedFlag, sharedListProcessos, *id);           // Distribui os processos
    printf("\nProcesso AUX(%d): FIM DA DISTRIBUIÇÃO STRIPED....Auxiliar(%d)\n\n", getpid(), *id);
    
    if(stripedFlag[3] = 1) {                                // Se a distribuição já acabou
        if(*modo == 1) {                                     //
            // modo normal
            execNormal(tempo);                             //
            printf("Processo AUX(%d): --->>>>> TERMINEI meus processos...\n\n", getpid());
        }
        else if(*modo == 2) {                                //
            // modo roubo
            execNormal(tempo);                             //
            printf("Processo AUX(%d): --->>>>> TERMINEI meus processos, entrando em modo Roubo...\n\n", getpid());
            execRoubo(tempo);                              //
        }
    }

    shmdt(stripedFlag);                                     // Desanexa segmentos de memoria  
    shmdt(sharedListProcessos);                             //
}

// Responsável por ler o arquivo e inserir os processos na lista encadeada
int leArquivo(processo **lista, char *nomeArquivo) {
    char *read = malloc(sizeof(char) * 20);
    FILE *arqProcessos;

    arqProcessos = fopen(nomeArquivo, "r");

    if (arqProcessos == NULL) {
        printf("\nERRO! O arquivo de processos não existe, ou o diretorio esta incorreto!\n\n");
        exit(1);
    }
    
    int id = 0;
    while(fgets(read, 20, arqProcessos) != NULL) {
        int tam = strlen(read);
        read[tam-1] = '\0';
        minusculas(read);
        if(strlen(read) > 0) {
            insereProcesso(lista, id, read);
            id++;
        }
    }
    fclose(arqProcessos);

    if(id == 0) {
        printf("\nERRO! O arquivo de processos está vazio!\n\n");
        exit(1);
    }
    return id;
}

// Responsável por atulizar o tempo e o estado de um processo na lista de dados
void updateEstatistica(int id, int tempo, int status) {
    for(int i=0; i<numProcessos; i++) {
        if(listaEst[i].id == id) {
            listaEst[i].tempo = tempo;
            listaEst[i].status = status;
        }
    }
}

// Responsável por printar a tabela de dados
void printEstatistica(estatistica *lista, int *p_auxs) {
    char aux[30];
    printf("\n===============================================================================\n");
    printf("|Executor  |Dono      |Id     |Nome    |Pid       |Estado    |Tempo  |Situacao|\n");
    printf("===============================================================================\n|");

    for(int i=0; i<numProcessos; i++) {
        char esp1[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].idProcessoExec);
        esp1[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].idProcessoExec, esp1);

        char esp2[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].idDonoOriginal);
        esp2[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].idDonoOriginal, esp2);

        char esp3[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].id);
        esp3[7 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].id, esp3);

        char esp4[11] = { "          " };
        esp4[8 - strlen(listaEst[i].nome)] = '\0';
        printf("%s%s|", listaEst[i].nome, esp4);

        char esp5[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].pid);
        esp5[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].pid, esp5);
        
        if(listaEst[i].status == 1) {
            printf("Terminado |");    
        } else {
            printf("error     |");    
        }

        char esp6[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].tempo);
        esp6[6 - strlen(aux)] = '\0';
        printf("%ds%s|", listaEst[i].tempo, esp6);

        if(listaEst[i].exec == 1) {
            printf("Meu     |");    
        }
        else if(listaEst[i].exec == 2) {
            printf("Roubado |");    
        } else {
            printf("error   |");    
        }

        if (i == numProcessos-1) {
            printf("\n-------------------------------------------------------------------------------\n");
        } else {
            printf("\n-------------------------------------------------------------------------------\n|");
        }
    }

    printf("===============================================================================\n\n");

    printf("\n===============================================================================\n");
    printf("|Processo AUX  |Quantidade Executada |Roubou            |Tempo de Exec p/ Aux |\n");
    printf("===============================================================================\n|");

    int execs = 0;
    int robs = 0;
    int tempo = 0;

    for(int i=0; i<PROCESSOS_AUX; i++) {
        execs = 0;
        robs = 0;
        tempo = 0;

        for(int j=0; j<numProcessos; j++) {
            if( p_auxs[i] == listaEst[j].idProcessoExec) {
                execs++;
                if(listaEst[j].exec == 2) {
                    robs++;
                }
                tempo += listaEst[j].tempo;
            }
        }

        char esp1[15] = { "              " };
        snprintf(aux, sizeof(aux), "%d", p_auxs[i]);
        esp1[14 - strlen(aux)] = '\0';
        printf("%d%s|", p_auxs[i], esp1);

        char esp2[12] = { "           " };
        snprintf(aux, sizeof(aux), "%d", execs);
        //printf("Dono: %ld ", strlen(aux2));
        esp2[11 - strlen(aux)] = '\0';
        printf("%d Processos%s|", execs, esp2);

        char esp3[9] = { "        " };
        snprintf(aux, sizeof(aux), "%d", robs);
        esp3[8 - strlen(aux)] = '\0';
        printf("%d Processos%s|", robs, esp3);

        char esp4[13] = { "             " };
        snprintf(aux, sizeof(aux), "%d", tempo);
        esp4[12 - strlen(aux)] = '\0';
        printf("%d Segundos%s|", tempo, esp4);

        if (i == PROCESSOS_AUX-1) {
            printf("\n-------------------------------------------------------------------------------\n");
        } else {
            printf("\n-------------------------------------------------------------------------------\n|");
        }
    }
    printf("===============================================================================\n");
}

// Responsável por testar a entrada na linha de comando, afim de evitar erros
void testeArgumentos(int argc, char *argv[], int *modo) {
    if(argc < 3) {
        printf("\nArgumentos insuficientes:\n >>> Encontrado(%d): %s %s %s\n >>> Esperado(%d): ./escalonador ../seus_processos.txt modo(-normal ou -roubo)\n\n",argc, argv[0], argv[1], argv[2], 3);
        exit(1);
    } else {
        if( strcmp(argv[2], "-normal") == 0) {
            printf("\n>>>>>>>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);
            *modo = 1;
        }
        else if(strcmp(argv[2], "-roubo") == 0) {
            printf("\n>>>>>>>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);
            *modo = 2;
        } else {
            printf("\nERRO! Modo de execucao nao existe! Tente: '-normal' ou '-roubo'\n\n");
            exit(1);
        }
    }
}

// Responsável por inserir um processo na lista encadeada
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

// Responsável por retornar um processo, seja para o modo '1' (retorna para o dono e com estado pronto), ou, para o modo '2' (retorna o primeiro com estado pronto)
processo* buscaProcesso(processo *lista, pid_t pid, int modo) {
    int achou = 0;
    if (modo == 1) {
        while(lista) {
            if(lista->donoProcesso == pid && lista->estado == 0) {
                achou = 1;
                break;
            }
            else if(lista->donoProcesso == pid && lista->estado == 3) {
                achou = 1;
                break;
            }
            lista = lista->prox;
        }
        if(achou == 1) {
            return lista;
        }
        return NULL;
    }

    else if (modo == 2) {
        while(lista) {
            if(lista->estado == 0) {
                achou = 1;
                break;
            }
            lista = lista->prox;
        }

        if(achou == 1) {
            return lista;
        }
        return NULL;
    }
    return NULL;
}

// Responsável por definir o dono do processo
int atribuiProcesso(int striped_dois, processo *sharedListProcessos) {
    // sharedListProcessos -> Ponteiro de inicio da lista de processos.
    if (striped_dois < numProcessos) {                              // Verifico se o contador que inicia zerado, é menor que o numero total de processos.
        sharedListProcessos = sharedListProcessos + striped_dois;   // Adiciono o numero do contador ao ponteiro da lista de processos, avancando pro endereco correto.
                                                                    // Ex: Se estamos no Auxiliar 0, e o contador é 3, ele vai pegar o endereco inicial e add 3, pegando o processo sem dono,
                                                                    // pois os processos anteriores ja se atribuiram. Isso ocorre porque o acesso é cicular, vai ser sempre 
                                                                    // processo aux 0, aux 1, aux 2, aux 3, e depois volta ao aux 0...
        sharedListProcessos->donoProcesso = getpid();               // Atribui o pid do processo auxiliar a um processo na lista
        striped_dois++;                                             // Incrementa o contador de processos ja atribuidos
    }

    return striped_dois;                                            // Retorno o contador atualizado
}

// Responsável por copiar a lista encadeana na memoria, para outra lista encadeada no segmento de memoria compartilhada
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
            lista = lista->prox;
        }
    }
}

// Responsável por inserir os dados de execução de um processo na tabela de dados
void insereEstatistica(int pidExecutor, processo *aux, int pid, int exec) {
    for(int i=0; i<numProcessos; i++) {
        if(listaEst[i].ocupado == 0) {
            listaEst[i].idProcessoExec = pidExecutor;
            listaEst[i].idDonoOriginal = aux->donoProcesso;
            listaEst[i].id = aux->id;
            strcpy(listaEst[i].nome, aux->nome);
            listaEst[i].pid = pid;
            listaEst[i].tempo = 0;
            listaEst[i].status = -1;
            listaEst[i].exec = exec;    // 1 == meu processo, 3 == roubado
            listaEst[i].ocupado = 1;
            break;
        }
    }
}

// Responsável pela distribuição modo Striped
void striped(int *stripedFlag, processo *sharedListProcessos, int processoAux) {
    
    while (stripedFlag[0] == -1) {                  

        if (stripedFlag[2] > numProcessos - 1) {                                // Se o contador de processos for maior que ultimo processo da lista,
            stripedFlag[0] = 1;                                                 // ele finaliza o while principal, ou seja, terminou a distribuição
            stripedFlag[3] = 1;                                                 // Da o sinal para finalizar os processos AUX em busy waiting
            break;
        }

        while(stripedFlag[1] != processoAux && stripedFlag[3] == 0);            // BUSY WAITING
                                                // Espera ate ser a sua vez de se atribuir um processo, no caso ate striped[1] ser ser o indice do processo AUX
                                                // Importante ressaltar que o processo Aux 0 nunca passa daqui até ser sua vez novamente, ou seja, ate que
                                                // outro processo Aux diga que é sua vez "striped[1] = processoAux (indice no vetor de PIDS);"
                                                // ou até que distribuição termine
        
        stripedFlag[2] = atribuiProcesso(stripedFlag[2], sharedListProcessos);  // Atrubui o processo a um dono, e retorna a quantidade atual de processos ja
                                                                                // distribuidos
        
        if (processoAux != PROCESSOS_AUX - 1) {     // Se não estivermos no ultimo processo auxiliar
            stripedFlag[1] = processoAux + 1;       // Define o numero do proximo processo auxiliar a se atribuir um processo
        } else {
            stripedFlag[1] = 0;                     // Estamos no ultimo processo auxiliar, entao voltamos ao processo auxiliar 0
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

    time_t inicio = time(NULL);                                                 // Marca do tempo de inicio (makespan)

    int status;                                                                 // Variavel status para "Wait" processos AUX
    int modo = 0;                                                               // Variavel para definir o modo de execução, normal ou roubo
    int tempo = 0;                                                              // Variavel para calcular tempos individuais dos processos AUX
    pid_t p_auxs[PROCESSOS_AUX];                                                // Vetor de processos AUX, seus respectivos PIDs
    processo *listaProcessos = NULL;                                            // Lista encadeada de processos, ainda memoria local

    testeArgumentos(argc, argv, &modo);                                         //

    numProcessos = leArquivo(&listaProcessos, argv[1]);                         // Atribui o total de processos a serem executados
    printf("Processos para execução: %d\n", numProcessos);
    
    alocaIPCs();
    signal(SIGTERM, limpeza);                                                   // Sinais que aguardam caso seja necessário finalizar o programa por interrupções
    signal(SIGINT, limpeza);                                                    // Caso aconteça, os segmentos e semaforos são removidos da memoria.
    signal(SIGSEGV, limpeza);                                                   //

    stripedFlag[0] = -1;                                                        // Flag para finalizar a distribuicao dos processos
    stripedFlag[1] = 0;                                                         // Valor de alternancia para garantir a ordem correta na distribuicao
    stripedFlag[2] = 0;                                                         // Contador para percorrer a lista de processos
    stripedFlag[3] = 0;                                                         // Flag para encerrar processos em busy waiting
    
    for(int i=0; i<numProcessos; i++) {                                         // Inicializa a tabela de dados
        listaEst[i].ocupado = 0;                                                // Marca como disponivel
    }                                                                           //

    listaParaListaCompartilhada(listaProcessos, sharedListProcessos);           //
    liberaListaProcessos(&listaProcessos);                                      //

    for(int i=0; i<PROCESSOS_AUX; i++) {                                        // Escalonador(PAI) cria processos Auxiliares (FILHOS)
        p_auxs[i] = fork();                                                     //         

        if (p_auxs[i] == 0) {                                                   // Código dos processos Auxiliares (FILHOS)
            
            switch (i) {
                case 0:                                                         // Caso esteja no Auxiliar (0)
                        executaAUX(&tempo, &modo, &i);                          // Executa código do Auxiliar (0)
                        _exit(0);                                               // Avisa ao Escalonador(PAI) que terminou, retorna o id do AUX
                        break;
                
                case 1:                                                         // Caso esteja no Auxiliar (1)
                        executaAUX(&tempo, &modo, &i);                          // Executa código do Auxiliar (1)
                        _exit(1);                                               // Avisa ao Escalonador(PAI) que terminou, retorna o id do AUX
                        break;
                
                case 2:                                                         // Caso esteja no Auxiliar (1)
                        executaAUX(&tempo, &modo, &i);                          // Executa código do Auxiliar (2)
                        _exit(2);                                               // Avisa ao Escalonador(PAI) que terminou, retorna o id do AUX
                        break;
                
                case 3:                                                         // Caso esteja no Auxiliar (1)
                        executaAUX(&tempo, &modo, &i);                          // Executa código do Auxiliar (3)
                        _exit(3);                                               // Avisa ao Escalonador(PAI) que terminou, retorna o id do AUX
                        break;
                
                default:
                        break;
            }
        }
    }

    printf("\nESCALONADOR(%d): Meus filhos auxiliares são:\n\n", getpid());
    for (int i=0; i<PROCESSOS_AUX; i++) {
        if(i == PROCESSOS_AUX-1) {
            printf("AUX(%d)\n\n", p_auxs[i]);
        } else {
            printf("AUX(%d), ", p_auxs[i]);
        }
    }

    for(int i=0; i<PROCESSOS_AUX; i++) {                                        // Loop para esperar por todos os processos Auxiliares terminarem
        pid_t child_pid = wait(&status);                                        // Espera aqui pelo termino de algum AUX
        switch (WEXITSTATUS(status)) {
                case 0:
                        printf("ESCALONADOR(%d): Processo Auxiliar(%d) -- Pid: %d , Morreu..\n\n", getpid(), WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 1:
                        printf("ESCALONADOR(%d): Processo Auxiliar(%d) -- Pid: %d , Morreu..\n\n", getpid(), WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 2:
                        printf("ESCALONADOR(%d): Processo Auxiliar(%d) -- Pid: %d , Morreu..\n\n", getpid(), WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;
                
                case 3:
                        printf("ESCALONADOR(%d): Processo Auxiliar(%d) -- Pid: %d , Morreu..\n\n", getpid(), WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                default:
                        break;
        }  
    }
    
    printEstatistica(listaEst, p_auxs);
    
    printf("\n>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);

    time_t fim = time(NULL);                                                    // Marca do tempo de fim da execução (makespan)
    printf("\n>>>>>>>>>>>>>>>>>> Tempo total de execucao da Aplicacao (Makespan): %ld Segundos\n", (fim - inicio));

    limpeza();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////