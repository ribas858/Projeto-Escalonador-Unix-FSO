#ifndef FUNCS_H
#define FUNCS_H

#include<stdio.h>
#include<string.h>
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

#define PROCESSOS_AUX 2



int idsem;
struct sembuf operacao[2];



typedef struct no {
    int id;
    char nome[20];
    int estado;         // 0 == Pronto, 1 == Terminou de executar
    pid_t donoProcesso; // dono == PID, sem dono == -1
    struct no *prox;
} processo;

int idMemoriaProcessos;
int idMemoriaStripedVetor;
int idMemoriaTempo;

processo *sharedListProcessos;
int *stripedFlag;
int *tempoTotal;

void limpeza();

int p_sem();
int v_sem();
void insereProcesso(processo **lista, int id, char *str);
void printProcessos(processo *lista);
processo* buscaProcesso(processo *lista, pid_t pid);
void listaParaListaCompartilhada(processo *lista, processo *listaCompart);
void liberaListaProcessos(processo **lista);
int atribuiProcesso(int striped_dois, processo *sharedListProcessos, int numProcessos);
int leArquivo(processo **lista, char *nomeArquivo);

void striped(int *stripedFlag, processo *sharedListProcessos, int numProcessos, int processoAux);

int meusProcessos(processo *lista, pid_t pid);


#endif