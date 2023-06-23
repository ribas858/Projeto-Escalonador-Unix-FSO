#include "funcs.h"

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