#include "funcs.h"



int main(int argc, char *argv[]) {
    
    signal(SIGTERM, limpeza);
    signal(SIGINT, limpeza);

    

    if (( idsem = semget(0x1223, 1, IPC_CREAT | 0x1ff) ) < 0) {
        printf("erro na criacao do semaforo\n");
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
    int status[PROCESSOS_AUX];
    
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
                        
                        //printf("Qtd dos meus processos: %d\n", qtdProcessos);
                        
                        if(stripedFlag[3] = 1) {
                            //printf("Procurando Processo %d\n", getpid());
                            p_sem();
                            int qtdProcessos = meusProcessos(sharedListProcessos, getpid());
                            v_sem();

                            while (qtdProcessos > 0) {

                                p_sem();
                                processo *aux = buscaProcesso(sharedListProcessos, getpid());
                                v_sem();

                                if(aux) {
                                        printf("Achou processo..%d\n", aux->id);
                                        
                                        pid_t execPid = fork();
                                        
                                        printf("PID NETO: %d\n", getpid());
                                        char path[50] = {"processos/"};
                                        strcat(path, aux->nome);

                                        time_t begin = time(NULL);
                                        if(execPid == 0) {
                                            execl(path, aux->nome, (char *) NULL);
                                        }
                                        wait(NULL);
                                        time_t end = time(NULL);
                                        
                                        p_sem();
                                        aux->estado = 1;
                                        qtdProcessos--;
                                        v_sem();

                                        tempo = end - begin;
                                        printf("\n----------------->>>>> FIM EXEC.... Tempo %d Segundos\n", tempo);
                                        *tempoTotal += tempo;
                                }
                            }
                            
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        exit(i);
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

                        //printf("Qtd dos meus processos: %d\n", qtdProcessos);
                        
                        if(stripedFlag[3] = 1) {
                            //printf("Procurando Processo %d\n", getpid());
                            p_sem();
                            int qtdProcessos = meusProcessos(sharedListProcessos, getpid());
                            v_sem();

                            while (qtdProcessos > 0) {

                                p_sem();
                                processo *aux = buscaProcesso(sharedListProcessos, getpid());
                                v_sem();

                                if(aux) {
                                        printf("Achou processo..%d\n", aux->id);
                                        
                                        pid_t execPid = fork();
                                        
                                        printf("PID NETO: %d\n", getpid());
                                        char path[50] = {"processos/"};
                                        strcat(path, aux->nome);

                                        time_t begin = time(NULL);
                                        if(execPid == 0) {
                                            execl(path, aux->nome, (char *) NULL);
                                        }
                                        wait(NULL);
                                        time_t end = time(NULL);
                                        
                                        p_sem();
                                        aux->estado = 1;
                                        qtdProcessos--;
                                        v_sem();

                                        tempo = end - begin;
                                        printf("\n----------------->>>>> FIM EXEC.... Tempo %d Segundos\n", tempo);
                                        *tempoTotal += tempo;
                                }
                            }
                            
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        exit(i);
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

                        //printf("Qtd dos meus processos: %d\n", qtdProcessos);
                        
                        if(stripedFlag[3] = 1) {
                            //printf("Procurando Processo %d\n", getpid());
                            p_sem();
                            int qtdProcessos = meusProcessos(sharedListProcessos, getpid());
                            v_sem();

                            while (qtdProcessos > 0) {

                                p_sem();
                                processo *aux = buscaProcesso(sharedListProcessos, getpid());
                                v_sem();

                                if(aux) {
                                        printf("Achou processo..%d\n", aux->id);
                                        
                                        pid_t execPid = fork();
                                        
                                        printf("PID NETO: %d\n", getpid());
                                        char path[50] = {"processos/"};
                                        strcat(path, aux->nome);

                                        time_t begin = time(NULL);
                                        if(execPid == 0) {
                                            execl(path, aux->nome, (char *) NULL);
                                        }
                                        wait(NULL);
                                        time_t end = time(NULL);
                                        
                                        p_sem();
                                        aux->estado = 1;
                                        qtdProcessos--;
                                        v_sem();

                                        tempo = end - begin;
                                        printf("\n----------------->>>>> FIM EXEC.... Tempo %d Segundos\n", tempo);
                                        *tempoTotal += tempo;
                                }
                            }
                            
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

                        _exit(i);
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

                        //printf("Qtd dos meus processos: %d\n", qtdProcessos);
                        
                        if(stripedFlag[3] = 1) {
                            //printf("Procurando Processo %d\n", getpid());
                            p_sem();
                            int qtdProcessos = meusProcessos(sharedListProcessos, getpid());
                            v_sem();

                            while (qtdProcessos > 0) {

                                p_sem();
                                processo *aux = buscaProcesso(sharedListProcessos, getpid());
                                v_sem();

                                if(aux) {
                                        printf("Achou processo..%d\n", aux->id);
                                        
                                        pid_t execPid = fork();
                                        
                                        printf("PID NETO: %d\n", getpid());
                                        char path[50] = {"processos/"};
                                        strcat(path, aux->nome);

                                        time_t begin = time(NULL);
                                        if(execPid == 0) {
                                            execl(path, aux->nome, (char *) NULL);
                                        }
                                        wait(NULL);
                                        time_t end = time(NULL);
                                        
                                        p_sem();
                                        aux->estado = 1;
                                        qtdProcessos--;
                                        v_sem();

                                        tempo = end - begin;
                                        printf("\n----------------->>>>> FIM EXEC.... Tempo %d Segundos\n", tempo);
                                        *tempoTotal += tempo;
                                }
                            }
                            
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);
                        shmdt(tempoTotal);

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
    
    //sleep(20);

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

    printf("\n>>>>>>>>>>>>>>>>>>>>>>>> Tempo total de execução: %d Segundos\n", *tempoTotal);

    

    limpeza();

    return 0;
}