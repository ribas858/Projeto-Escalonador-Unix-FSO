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

#define PROCESSOS_AUX 4 // Máximo 4 processos auxiliares

typedef struct no {
    int id;
    int estado;             // 0 == Pronto, 1 == Terminou de executar
    char nome[20];
    struct no *prox;
    pid_t donoProcesso;     // dono == PID, sem dono == -1
} processo;

typedef struct est {
    int id;
    int pid;
    int exec;               // 1 == meu processo, 2 == roubado
    int tempo;              // tempo individual
    int status;             // 1 == Terminou de executar
    int ocupado;            // 1 == ocupado, 0 disponivel
    char nome[20];
    int idProcessoExec;
    int idDonoOriginal;
} estatistica;

extern struct sembuf operacao[2];

extern int idSemaforo;
extern int numProcessos;
extern int idMemoriaEst;
extern int idMemoriaExecutou;
extern int idMemoriaProcessos;
extern int idMemoriaStripedVetor;

extern int *executou;
extern int *stripedFlag;
extern estatistica *listaEst;
extern processo *sharedListProcessos;

extern key_t semKey;
extern key_t memo1Key;
extern key_t memo2Key;
extern key_t memo3Key;


int p_sem();
int v_sem();
void limpeza();
void alocaIPCs();
void limpezaExec();
void execRoubo(int *tempo);
void execNormal(int *tempo);
void minusculas(char *read);
void printProcessos(processo *lista);
void liberaListaProcessos(processo **lista);
int meusProcessos(processo *lista, pid_t pid);
int leArquivo(processo **lista, char *nomeArquivo);
void updateEstatistica(int id, int tempo, int status);
void printEstatistica(estatistica *lista, int *p_auxs);
void testeArgumentos(int argc, char *argv[], int *modo);
void insereProcesso(processo **lista, int id, char *str);
processo* buscaProcesso(processo *lista, pid_t pid, int modo);
int atribuiProcesso(int striped_dois, processo *sharedListProcessos);
void listaParaListaCompartilhada(processo *lista, processo *listaCompart);
void insereEstatistica(int pidExecutor, processo *aux, int pid, int exec);
void striped(int *stripedFlag, processo *sharedListProcessos, int processoAux);

#endif