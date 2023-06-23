#include "funcs.h"

int idSemaforo;
int numProcessos;
int idMemoriaEst;
int idMemoriaExecutou;
int idMemoriaProcessos;
int idMemoriaStripedVetor;

int *executou;
int *stripedFlag;
estatistica *listaEst;
processo *sharedListProcessos;

key_t semKey = 0x00000862;
key_t memo1Key = 0x00000858;
key_t memo2Key = 0x00000859;
key_t memo3Key = 0x00000860;

struct sembuf operacao[2];

int main(int argc, char *argv[]) {

    time_t inicio = time(NULL);

    int status;
    int modo = 0;
    int tempo = 0;
    pid_t p_auxs[PROCESSOS_AUX];
    processo *listaProcessos = NULL;

    testeArgumentos(argc, argv, &modo);

    numProcessos = leArquivo(&listaProcessos, argv[1]);
    printf("Processos para execução: %d\n", numProcessos);
    
    alocaIPCs();
    signal(SIGTERM, limpeza);
    signal(SIGINT, limpeza);
    signal(SIGSEGV, limpeza);

    stripedFlag[0] = -1;    // Flag para finalizar a distribuicao dos processos
    stripedFlag[1] = 0;     // Valor de alternancia para garantir a ordem correta na distribuicao
    stripedFlag[2] = 0;     // Contador para percorrer a lista de processos
    stripedFlag[3] = 0;     // Contador para percorrer a lista de processos
    
    for(int i=0; i<numProcessos; i++) {
        listaEst[i].ocupado = 0;
    }

    listaParaListaCompartilhada(listaProcessos, sharedListProcessos);
    liberaListaProcessos(&listaProcessos);

    for(int i=0; i<PROCESSOS_AUX; i++) {
        p_auxs[i] = fork();

        if (p_auxs[i] == 0) {
            
            switch (i) {
                case 0:
                        p_sem();
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);
                        printf("\nProcesso AUX(%d): FIM DA DISTRIBUIÇÃO STRIPED....Auxiliar(%d)\n\n", getpid(), i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                // modo normal
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos...\n\n", getpid());
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos, entrando em modo Roubo...\n\n", getpid());
                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        exit(0);
                        break;
                
                case 1:
                        p_sem();
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nProcesso AUX(%d): FIM DA DISTRIBUIÇÃO STRIPED....Auxiliar(%d)\n\n", getpid(), i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos...\n\n", getpid());
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos, entrando em modo Roubo...\n\n", getpid());
                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        exit(1);
                        break;
                
                case 2:
                        p_sem();
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nProcesso AUX(%d): FIM DA DISTRIBUIÇÃO STRIPED....Auxiliar(%d)\n\n", getpid(), i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos...\n\n", getpid());
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos, entrando em modo Roubo...\n\n", getpid());
                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        _exit(2);
                        break;
                
                case 3:
                        p_sem();
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        v_sem();
                        
                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nProcesso AUX(%d): FIM DA DISTRIBUIÇÃO STRIPED....Auxiliar(%d)\n\n", getpid(), i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos...\n\n", getpid());
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                printf("Processo AUX(%d): --->>>>> TERMINEI meus processos, entrando em modo Roubo...\n\n", getpid());
                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        _exit(3);
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

    for(int i=0; i<PROCESSOS_AUX; i++) {
        pid_t child_pid = wait(&status);
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

    time_t fim = time(NULL);
    printf("\n>>>>>>>>>>>>>>>>>> Tempo total de execucao da Aplicacao (Makespan): %ld Segundos\n", (fim - inicio));

    limpeza();
}
