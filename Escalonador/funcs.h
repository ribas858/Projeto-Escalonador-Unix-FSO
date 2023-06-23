#ifndef FUNCS_H
#define FUNCS_H

#include<stdio.h>
#include<string.h>
#include <ctype.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/wait.h>

#include<unistd.h>
#include <time.h>

#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<errno.h>
#include<signal.h>

#define PROCESSOS_AUX 4
extern int numProcessos;

extern key_t memo1Key;
extern key_t memo2Key;
extern key_t memo3Key;
//extern key_t memo4Key;

extern key_t semKey;

extern struct sembuf operacao[2];
extern int idSemaforo;



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

extern int idMemoriaProcessos;
extern int idMemoriaStripedVetor;
extern int idMemoriaEst;
extern int idMemoriaExecutou;

extern processo *sharedListProcessos;
extern int *stripedFlag;
extern estatistica *listaEst;
extern int *executou;


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
