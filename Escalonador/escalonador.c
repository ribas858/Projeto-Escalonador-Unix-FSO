#include "funcs.h"

int numProcessos;

struct sembuf operacao[2];
int idSemaforo;

key_t memo1Key;
key_t memo2Key;
key_t memo3Key;
key_t memo4Key;
key_t semKey;

int idMemoriaProcessos;
int idMemoriaStripedVetor;
int idMemoriaEst;
int idMemoriaExecutou;

processo *sharedListProcessos;
int *stripedFlag;
estatistica *listaEst;
int *executou;


int main(int argc, char *argv[]) {

    int status;
    int modo = 0;
    int tempo = 0;

    key_t memo1Key = 0x00000858;
    key_t memo2Key = 0x00000859;
    key_t memo3Key = 0x00000860;
    key_t memo4Key = 0x00000861;
    key_t semKey = 0x00000862;

    pid_t p_auxs[PROCESSOS_AUX];
    processo *listaProcessos = NULL;

    testeArgumentos(argc, argv, &modo);

    numProcessos = leArquivo(&listaProcessos, argv[1]);
    printf("Processos: %d\n", numProcessos);
    //printProcessos(listaProcessos);
    
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
    //printProcessos(sharedListProcessos);

    time_t inicio = time(NULL);

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
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);
                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                // modo normal
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                
                                printf("\nTerminei meus processos...\n");

                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        exit(0);
                        break;
                
                case 1:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                
                                printf("\nTerminei meus processos...\n\n");

                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        exit(1);
                        break;
                
                case 2:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();

                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                
                                printf("\nTerminei meus processos...\n");

                                execRoubo(&tempo);
                            }
                        }

                        shmdt(stripedFlag);
                        shmdt(sharedListProcessos);

                        _exit(2);
                        break;
                
                case 3:
                        printf("\n========================FILHO %d\n", i);
                        p_sem();
                        // printf("filho %d - obtive o semaforo\n", i);
                        stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);
                        sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);
                        // printf("filho %d - vou liberar o semaforo\n", i);
                        v_sem();
                        
                        striped(stripedFlag, sharedListProcessos, i);

                        printf("\nFIM DA DISTRIBUIÇÃO....%d\n", i);
                        
                        if(stripedFlag[3] = 1) {
                            if(modo == 1) {
                                execNormal(&tempo);
                            }
                            else if(modo == 2) {
                                // modo roubo
                                execNormal(&tempo);
                                
                                printf("\nTerminei meus processos...\n");

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
    // printf("\n================= FIM DO For de Forks =================\n");

    printf("\nPAI(%d), meus filhos são: ", getpid());
    for (int i=0; i<PROCESSOS_AUX; i++) {
        printf("%d e ", p_auxs[i]);
    }
    printf("\n\n");
    
    //sleep(20);

    for(int i=0; i<PROCESSOS_AUX; i++) {
        pid_t child_pid = wait(&status);
        
        switch (WEXITSTATUS(status)) {
                case 0:
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 1:
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                case 2:
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;
                
                case 3:
                        printf("\nMeu filho(%d) pid: %d , Morreu..\n\n", WEXITSTATUS(status), p_auxs[WEXITSTATUS(status)]);
                        break;

                default:
                        break;
        }  
    }
    time_t fim = time(NULL);
    // printf("PASSOU DO WAIT >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> status: %d\n", WEXITSTATUS(status));

    printProcessos(sharedListProcessos);

   
    printEstatistica(listaEst, p_auxs);
    
    printf("\n>>>>>>>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);
    printf("\n>>>>>>>>>>>>>>>>>>>>>>>> Tempo total de execucao da Aplicacao: %ld Segundos\n", (fim - inicio));

    

    limpeza();

    return 0;
}
