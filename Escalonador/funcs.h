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

extern struct sembuf operacao[2];

typedef struct no {
    int id;
    char nome[20];
    int estado;         // 0 == Pronto, 1 == Terminou de executar
    pid_t donoProcesso; // dono == PID, sem dono == -1
    struct no *prox;
} processo;

typedef struct est {
    int ocupado;    // 1 == ocupado, 0 disponivel
    int idProcessoExec;
    int idDonoOriginal;
    int id;
    char nome[20];
    int pid;
    int tempo;      // tempo individual
    int status;     // 1 == Terminou de executar
    int exec;       // 1 == meu processo, 2 == roubado
} estatistica;


void testeArgumentos(int argc, char *argv[], int *modo);
int leArquivo(processo **lista, char *nomeArquivo);

void alocaIPCs();

void minusculas(char *read);

void limpeza();

int p_sem();
int v_sem();

void insereProcesso(processo **lista, int id, char *str);
void printProcessos(processo *lista);
int atribuiProcesso(int striped_dois, processo *sharedListProcessos);
processo* buscaProcesso(processo *lista, pid_t pid, int modo);
int meusProcessos(processo *lista, pid_t pid);

void listaParaListaCompartilhada(processo *lista, processo *listaCompart);
void liberaListaProcessos(processo **lista);

void striped(int *stripedFlag, processo *sharedListProcessos, int processoAux);

void execNormal(int *tempo);

void execRoubo(int *tempo);

void insereEstatistica(int pidExecutor, processo *aux, int pid, int exec);
void updateEstatistica(int id, int tempo, int status);
void printEstatistica(estatistica *lista, int *p_auxs);


#endif
