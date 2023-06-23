#ifndef FUNCS_H
#define FUNCS_H

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

extern struct sembuf operacao[2];                                               // Estrutura de operações com Semaforos

extern int idSemaforo;                                                          // Id do semaforo
extern int numProcessos;                                                        // Numero total de processos a serem executados
extern int idMemoriaEst;                                                        // Id da memoria compartilhada 'listaEst'
extern int idMemoriaExecutou;                                                   // Id da memoria compartilhada 'executou'
extern int idMemoriaProcessos;                                                  // Id da memoria compartilhada 'sharedListProcessos'
extern int idMemoriaStripedVetor;                                               // Id da memoria compartilhada 'stripedFlag'

extern int *executou;                                                           // Variavel auxiliar no EXECL()
extern int *stripedFlag;                                                        // Vetor de flags para distribuição striped
extern estatistica *listaEst;                                                   // Lista de dados de execução
extern processo *sharedListProcessos;                                           // Lista encadeada de processos, no segmento de memoria compartilhada

extern key_t semKey;                                                            // Chave para semaforo
extern key_t memo1Key;                                                          // Chave 1 para segmento de memoria compartilhada
extern key_t memo2Key;                                                          // Chave 2 para segmento de memoria compartilhada
extern key_t memo3Key;                                                          // Chave 3 para segmento de memoria compartilhada


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



#endif