#include "funcs.h"



int main(int argc, char *argv[]) {
    int modo = 0;
    if(argc < 3) {
        printf("\nArgumentos insuficientes:\n >>> Encontrado(%d): %s %s %s\n >>> Esperado(%d): ./escalonador ../seus_processos.txt modo(-normal ou -roubo)\n\n",argc, argv[0], argv[1], argv[2], 3);
        exit(1);
    } else {
        if( strcmp(argv[2], "-normal") == 0) {
            printf("Modo: %s\n", argv[2]);
            modo = 1;
        }
        else if(strcmp(argv[2], "-roubo") == 0) {
            printf("Modo: %s\n", argv[2]);
            modo = 2;
        } else {
            printf("\nERRO! Modo de execucao nao existe! Tente: '-normal' ou '-roubo'\n\n");
            exit(1);
        }
    }
    
    signal(SIGTERM, limpeza);
    signal(SIGINT, limpeza);

    

    if (( idsem = semget(0x1223, 1, IPC_CREAT | 0x1ff) ) < 0) {
        printf("ERRO! Semaforo não foi criado!\n");
        exit(1);
    }

    ///////////////////////////////////////////////////////////////////////////
    printf("bytes por processo %ld\n", sizeof(processo));
    FILE *arq;
    char text[100];
    arq = fopen("log.txt", "w");
    sprintf(text, "%d", idsem);
    text[strlen(text)] = '\n';
    fprintf(arq, "%s", text);
    fclose(arq);
    ///////////////////////////////////////////////////////////////////////////

    processo *listaProcessos = NULL;
    //processo *sharedListProcessos;
    
    int numProcessos = leArquivo(&listaProcessos, argv[1]);
    printf("Processos: %d\n", numProcessos);

    //printProcessos(listaProcessos);

    idMemoriaProcessos = shmget (IPC_PRIVATE, sizeof(processo) * numProcessos, 0x1ff | IPC_CREAT);
    sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);

    idMemoriaStripedVetor = shmget(IPC_PRIVATE, sizeof(int) * 4,  0x1ff | IPC_CREAT);
    //int *stripedFlag;
    stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
    stripedFlag[0] = -1;    // Flag para finalizar a distribuicao dos processos
    stripedFlag[1] = 0;     // Valor de alternancia para garantir a ordem correta na distribuicao
    stripedFlag[2] = 0;     // Contador para percorrer a lista de processos

    stripedFlag[3] = 0;     // Contador para percorrer a lista de processos
    int tempo = 0;
    
    idMemoriaTempo = shmget(IPC_PRIVATE, sizeof(int),  0x1ff | IPC_CREAT);
    tempoTotal = (int *) shmat(idMemoriaTempo, NULL, 0);
    *tempoTotal = 0;

    listaParaListaCompartilhada(listaProcessos, sharedListProcessos);
    liberaListaProcessos(&listaProcessos);

    //printProcessos(sharedListProcessos);

    pid_t p_auxs[PROCESSOS_AUX];
    int status;
    
    for(int i=0; i<PROCESSOS_AUX; i++) {
        p_auxs[i] = fork();
        if (p_auxs[i] == 0) {
            
            switch (i) {
                case 0:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        tempoTotal = (int *) shmat(idMemoriaTempo, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, numProcessos, i);
                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // roubo
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        exit(0);
                        break;
                
                case 1:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        tempoTotal = (int *) shmat(idMemoriaTempo, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, numProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // roubo
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        exit(1);
                        break;
                
                case 2:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        tempoTotal = (int *) shmat(idMemoriaTempo, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, numProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // roubo
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        _exit(2);
                        break;
                
                case 3:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        tempoTotal = (int *) shmat(idMemoriaTempo, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();
                        
                        striped(stripedFlag, sharedListProcessos, numProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // roubo
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        _exit(3);
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
    
    //sleep(20);

    for(int i=0; i<PROCESSOS_AUX; i++) {
        switch (WEXITSTATUS(status)) {
                case 0:
                        wait(&status);
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 1:
                        wait(&status);
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 2:
                        wait(&status);
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;
                
                case 3:
                        wait(&status);
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                default:
                        break;
        }  
    }

    printProcessos(sharedListProcessos);

    printf("\n>>>>>>>>>>>>>>>>>>>>>>>> Tempo total de execução: %d Segundos\n", *tempoTotal);

    

    limpeza();

    return 0;
}