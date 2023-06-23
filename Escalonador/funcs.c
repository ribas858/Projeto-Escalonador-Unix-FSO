#include "funcs.h"

int p_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = 0;
    operacao[0].sem_flg = 0;

    operacao[1].sem_num = 0;
    operacao[1].sem_op = 1;
    operacao[1].sem_flg = 0;
    if (semop(idSemaforo, operacao, 2) < 0) {
        printf("Erro PSEM no p=%d\n", errno);
    }
}

int v_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = -1;
    operacao[0].sem_flg = 0;
    if (semop(idSemaforo, operacao, 1) < 0) {
        perror("Erro V");
        printf("Erro VSEM no p=%d\n", errno);
    }
}

void limpeza() {
    printf("\nESCALONADOR(): Liberando memória.....\n");

    if (semctl(idSemaforo, 0, IPC_RMID, operacao) < 0) {
        printf("Erro ao destruir Semaforos...\n");
    }
    if(shmdt(stripedFlag) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmdt(sharedListProcessos) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmdt(listaEst) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaStripedVetor, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaProcessos, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }
    if(shmctl(idMemoriaEst, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    }

    exit(0);
}

void alocaIPCs() {
    idSemaforo = semget(semKey, 1, IPC_CREAT | 0x1ff);
    if (idSemaforo < 0) {
        printf("ERRO! Semaforo não foi criado!\n");
        exit(1);
    }

    idMemoriaProcessos = shmget (memo1Key, sizeof(processo) * numProcessos, 0x1ff | IPC_CREAT);
    if (idMemoriaProcessos < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaProcessos' não foi criado!\n");
        exit(1);
    }
    sharedListProcessos = (processo *) shmat(idMemoriaProcessos, NULL, 0);

    idMemoriaStripedVetor = shmget(memo2Key, sizeof(int) * 4,  0x1ff | IPC_CREAT);
    if (idMemoriaStripedVetor < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaStripedVetor' não foi criado!\n");
        exit(1);
    }
    stripedFlag = (int *) shmat(idMemoriaStripedVetor, NULL, 0);

    idMemoriaEst = shmget (memo3Key, sizeof(estatistica) * numProcessos, 0x1ff | IPC_CREAT);
    if (idMemoriaEst < 0) {
        printf("ERRO! Segmento de memoria 'idMemoriaEst' não foi criado!\n");
        exit(1);
    }
    listaEst = (estatistica *) shmat(idMemoriaEst, NULL, 0);
}

void limpezaExec() {
    printf("\nLiberando memória.....EXEC\n");
    if(shmdt(executou) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada 'executou'...\n");
    }
    if(shmctl(idMemoriaExecutou, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada  'idMemoriaExecutou'...\n");
    }
    exit(0);
}

void execRoubo(int *tempo) {
    processo *aux = buscaProcesso(sharedListProcessos, 0, 2);
        while (aux) {
            printf("Processo AUX(%d): Roubei um processo - Dono: AUX(%d) - id: %d - estado: %d\n\n", getpid(), aux->donoProcesso, aux->id, aux->estado);
            
            p_sem();
            aux->estado = 2;
            v_sem();
            
            char path[50] = {"processos/"};
            strcat(path, aux->nome);

            idMemoriaExecutou = shmget (IPC_PRIVATE, sizeof(int), 0x1ff | IPC_CREAT);
            executou = (int *) shmat(idMemoriaExecutou, NULL, 0);
            *executou = 1;
            signal(SIGTERM, limpezaExec);
            signal(SIGINT, limpezaExec);
            signal(SIGSEGV, limpezaExec);

            time_t begin = time(NULL);
            pid_t execPid = fork();

            if(execPid == 0) {
                p_sem();
                insereEstatistica(getppid(), aux, getpid(), 2);
                v_sem();

                execl(path, aux->nome, (char *) NULL);

                executou = (int *) shmat(idMemoriaExecutou, NULL, 0);
                *executou = 0;

                shmdt(executou);
                exit(0);
            }

            wait(NULL);
            time_t end = time(NULL);
            
            if (*executou == 1) {
                p_sem();
                aux->estado = 3;
                v_sem();

                *tempo = end - begin;

                p_sem();
                updateEstatistica(aux->id, *tempo, 1);
                v_sem();
                
                printf("Processo AUX(%d): --->>>>> FIM EXEC - ROUBO '%s' Id(%d) Tempo %d Segundos\n\n", getpid(), aux->nome, aux->id, *tempo);
            } else {
                printf("----------------->>>>> ERRO! Processo '%s' Id(%d) NÃO EXECUTOU\n\n", aux->nome, aux->id);
                aux->estado = -1;
            }

            shmdt(executou);
            shmctl(idMemoriaExecutou, IPC_RMID, NULL);
            printf("Processo AUX(%d): Liberando memória....EXEC\n\n", getpid());

            aux = buscaProcesso(sharedListProcessos, 0, 2);
        }
        printf("Processo AUX(%d): Sem mais processos para executar, adeus...\n\n", getpid());
}

void execNormal(int *tempo) {

    p_sem();
    int qtdProcessos = meusProcessos(sharedListProcessos, getpid());
    v_sem();

    while (qtdProcessos > 0) {

        p_sem();
        processo *aux = buscaProcesso(sharedListProcessos, getpid(), 1);
        v_sem();

        if(aux) {
            if(aux->estado == 0) {
                printf("Processo AUX(%d): Encontrei meu processo..id(%d) -- estado(%d) 'Pronto'\n\n", getpid(), aux->id, aux->estado);
                p_sem();
                aux->estado = 2;    // Estado 2: Executando
                v_sem();
                
                char path[50] = {"processos/"};
                strcat(path, aux->nome);

                idMemoriaExecutou = shmget (IPC_PRIVATE, sizeof(int), 0x1ff | IPC_CREAT);
                executou = (int *) shmat(idMemoriaExecutou, NULL, 0);
                *executou = 1;
                signal(SIGTERM, limpezaExec);
                signal(SIGINT, limpezaExec);
                signal(SIGSEGV, limpezaExec);

                time_t begin = time(NULL);

                pid_t execPid = fork();
                if(execPid == 0) {

                    p_sem();
                    insereEstatistica(getppid(), aux, getpid(), 1);
                    v_sem();

                    execl(path, aux->nome, (char *) NULL);

                    executou = (int *) shmat(idMemoriaExecutou, NULL, 0);
                    *executou = 0;

                    shmdt(executou);
                    exit(0);
                }

                wait(NULL);
                time_t end = time(NULL);

                if (*executou == 1) {
                    p_sem();
                    aux->estado = 1;
                    qtdProcessos--;
                    v_sem();

                    *tempo = end - begin;

                    p_sem();
                    updateEstatistica(aux->id, *tempo, aux->estado);
                    v_sem();

                    printf("Processo AUX(%d): --->>>>> FIM EXEC.... '%s' Id(%d) Tempo %d Segundos\n\n", getpid(), aux->nome, aux->id, *tempo);
                } else {
                    printf("Processo AUX(%d): --->>>>> ERRO! Processo '%s' Id(%d) NÃO EXECUTOU\n\n", getpid(), aux->nome, aux->id);
                    aux->estado = -1;
                    qtdProcessos--;
                }

                shmdt(executou);
                shmctl(idMemoriaExecutou, IPC_RMID, NULL);
                printf("Processo AUX(%d): Liberando memória....EXEC\n\n", getpid());
            }
            else if(aux->estado == 3) {
                printf("Processo AUX(%d): Processo(%d) já executado por outro AUX. estado(%d) 'Roubado'\n\n", getpid(), aux->id, aux->estado);
                p_sem();
                aux->estado = 1;
                qtdProcessos--;
                v_sem();
            }
        }
    }
}

void minusculas(char *read) {
    int cont = 0;
    int tam = strlen(read);
    for(int i=0; i<tam; i++) {
        if(read[i] != ' ') {
            read[i] = tolower(read[i]);
            cont++;
        }
    }
    read[cont] = '\0';
}

void printProcessos(processo *lista) {
    while (lista) {
        printf("\nId: %d\n", lista->id);
        printf("Nome: %s\n", lista->nome);
        printf("Estado: %d\n", lista->estado);
        printf("Dono Processo: %d\n", lista->donoProcesso);
        lista = lista->prox;
    }
}

void liberaListaProcessos(processo **lista) {
    while(*lista) {
        processo *aux = (*lista)->prox;
        free(*lista);
        *lista = aux;
    }
}

int meusProcessos(processo *lista, pid_t pid) {
    int cont = 0;
    while (lista) {
        if(lista->donoProcesso == pid) {
            cont++;
        }
        lista = lista->prox;
    }
    return cont;
}

int leArquivo(processo **lista, char *nomeArquivo) {
    char *read = malloc(sizeof(char) * 20);
    FILE *arqProcessos;

    arqProcessos = fopen(nomeArquivo, "r");

    if (arqProcessos == NULL) {
        printf("\nERRO! O arquivo de processos não existe, ou o diretorio esta incorreto!\n\n");
        exit(1);
    }
    
    int id = 0;
    while(fgets(read, 20, arqProcessos) != NULL) {
        int tam = strlen(read);
        read[tam-1] = '\0';
        minusculas(read);
        if(strlen(read) > 0) {
            insereProcesso(lista, id, read);
            id++;
        }
    }
    fclose(arqProcessos);

    if(id == 0) {
        printf("\nERRO! O arquivo de processos está vazio!\n\n");
        exit(1);
    }
    return id;
}

void updateEstatistica(int id, int tempo, int status) {
    for(int i=0; i<numProcessos; i++) {
        if(listaEst[i].id == id) {
            listaEst[i].tempo = tempo;
            listaEst[i].status = status;
        }
    }
}

void printEstatistica(estatistica *lista, int *p_auxs) {
    char aux[30];
    printf("\n===============================================================================\n");
    printf("|Executor  |Dono      |Id     |Nome    |Pid       |Estado    |Tempo  |Situacao|\n");
    printf("===============================================================================\n|");

    for(int i=0; i<numProcessos; i++) {
        char esp1[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].idProcessoExec);
        esp1[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].idProcessoExec, esp1);

        char esp2[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].idDonoOriginal);
        esp2[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].idDonoOriginal, esp2);

        char esp3[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].id);
        esp3[7 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].id, esp3);

        char esp4[11] = { "          " };
        esp4[8 - strlen(listaEst[i].nome)] = '\0';
        printf("%s%s|", listaEst[i].nome, esp4);

        char esp5[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].pid);
        esp5[10 - strlen(aux)] = '\0';
        printf("%d%s|", listaEst[i].pid, esp5);
        
        if(listaEst[i].status == 1) {
            printf("Terminado |");    
        } else {
            printf("error     |");    
        }

        char esp6[11] = { "          " };
        snprintf(aux, sizeof(aux), "%d", listaEst[i].tempo);
        esp6[6 - strlen(aux)] = '\0';
        printf("%ds%s|", listaEst[i].tempo, esp6);

        if(listaEst[i].exec == 1) {
            printf("Meu     |");    
        }
        else if(listaEst[i].exec == 2) {
            printf("Roubado |");    
        } else {
            printf("error   |");    
        }

        if (i == numProcessos-1) {
            printf("\n-------------------------------------------------------------------------------\n");
        } else {
            printf("\n-------------------------------------------------------------------------------\n|");
        }
    }

    printf("===============================================================================\n\n");

    printf("\n===============================================================================\n");
    printf("|Processo AUX  |Quantidade Executada |Roubou            |Tempo de Exec p/ Aux |\n");
    printf("===============================================================================\n|");

    int execs = 0;
    int robs = 0;
    int tempo = 0;

    for(int i=0; i<PROCESSOS_AUX; i++) {
        execs = 0;
        robs = 0;
        tempo = 0;

        for(int j=0; j<numProcessos; j++) {
            if( p_auxs[i] == listaEst[j].idProcessoExec) {
                execs++;
                if(listaEst[j].exec == 2) {
                    robs++;
                }
                tempo += listaEst[j].tempo;
            }
        }

        char esp1[15] = { "              " };
        snprintf(aux, sizeof(aux), "%d", p_auxs[i]);
        esp1[14 - strlen(aux)] = '\0';
        printf("%d%s|", p_auxs[i], esp1);

        char esp2[12] = { "           " };
        snprintf(aux, sizeof(aux), "%d", execs);
        //printf("Dono: %ld ", strlen(aux2));
        esp2[11 - strlen(aux)] = '\0';
        printf("%d Processos%s|", execs, esp2);

        char esp3[9] = { "        " };
        snprintf(aux, sizeof(aux), "%d", robs);
        esp3[8 - strlen(aux)] = '\0';
        printf("%d Processos%s|", robs, esp3);

        char esp4[13] = { "             " };
        snprintf(aux, sizeof(aux), "%d", tempo);
        esp4[12 - strlen(aux)] = '\0';
        printf("%d Segundos%s|", tempo, esp4);

        if (i == PROCESSOS_AUX-1) {
            printf("\n-------------------------------------------------------------------------------\n");
        } else {
            printf("\n-------------------------------------------------------------------------------\n|");
        }
    }
    printf("===============================================================================\n");
}

void testeArgumentos(int argc, char *argv[], int *modo) {
    if(argc < 3) {
        printf("\nArgumentos insuficientes:\n >>> Encontrado(%d): %s %s %s\n >>> Esperado(%d): ./escalonador ../seus_processos.txt modo(-normal ou -roubo)\n\n",argc, argv[0], argv[1], argv[2], 3);
        exit(1);
    } else {
        if( strcmp(argv[2], "-normal") == 0) {
            printf("\n>>>>>>>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);
            *modo = 1;
        }
        else if(strcmp(argv[2], "-roubo") == 0) {
            printf("\n>>>>>>>>>>>>>>>>>>>>>>>> MODO: %s\n\n", argv[2]);
            *modo = 2;
        } else {
            printf("\nERRO! Modo de execucao nao existe! Tente: '-normal' ou '-roubo'\n\n");
            exit(1);
        }
    }
}

void insereProcesso(processo **lista, int id, char *str) {  
    processo *novoProcesso = malloc(sizeof(processo));

    if(novoProcesso) {
        
        novoProcesso->id = id;
        strcpy(novoProcesso->nome, str);

        novoProcesso->estado = 0;
        novoProcesso->donoProcesso = -1;
        novoProcesso->prox = NULL;

        if (*lista == NULL){
            *lista = novoProcesso;
        } else {
            processo *aux = malloc(sizeof(processo));
            aux = *lista;
            while (aux->prox) {
                aux = aux->prox;
            }
            aux->prox = novoProcesso;
        }

    } else {
        printf("Erro ao inserir processo\n");
    }
}

processo* buscaProcesso(processo *lista, pid_t pid, int modo) {
    int achou = 0;
    if (modo == 1) {
        while(lista) {
            if(lista->donoProcesso == pid && lista->estado == 0) {
                achou = 1;
                break;
            }
            else if(lista->donoProcesso == pid && lista->estado == 3) {
                achou = 1;
                break;
            }
            lista = lista->prox;
        }
        if(achou == 1) {
            return lista;
        }
        return NULL;
    }

    else if (modo == 2) {
        while(lista) {
            if(lista->estado == 0) {
                achou = 1;
                break;
            }
            lista = lista->prox;
        }

        if(achou == 1) {
            return lista;
        }
        return NULL;
    }
    return NULL;
}

int atribuiProcesso(int striped_dois, processo *sharedListProcessos) {
    // sharedListProcessos -> Ponteiro de inicio da lista de processos.
    if (striped_dois < numProcessos) {                              // Verifico se o contador que inicia zerado, é menor que o numero total de processos.
        sharedListProcessos = sharedListProcessos + striped_dois;   // Adiciono o numero do contador ao ponteiro da lista de processos, avancando pro endereco correto.
                                                                    // Ex: Se estamos no Auxiliar 0, e o contador é 3, ele vai pegar o endereco inicial e add 3, pegando o processo sem dono,
                                                                    // pois os processos anteriores ja se atribuiram. Isso ocorre porque o acesso é cicular, vai ser sempre 
                                                                    // processo aux 0, aux 1, aux 2, aux 3, e depois volta ao aux 0...
        sharedListProcessos->donoProcesso = getpid();               // Atribui o pid do processo auxiliar a um processo na lista
        striped_dois++;                                             // Incrementa o contador de processos ja atribuidos
    }

    return striped_dois;                                            // Retorno o contador atualizado
}

void listaParaListaCompartilhada(processo *lista, processo *listaCompart) {
    if(listaCompart == NULL) {
        printf("\nMemória compartilha não existe...\n\n");
    } else {
        processo *auxLista = listaCompart;
        while(lista) {
            auxLista->id = lista->id;
            strcpy(auxLista->nome, lista->nome);
            auxLista->estado = lista->estado;
            auxLista->donoProcesso = lista->donoProcesso;
            
            if(lista->prox == NULL) {
                auxLista->prox = NULL;    
            } else {
                auxLista->prox = auxLista + 1;
            }

            auxLista = auxLista + 1;
            lista = lista->prox;
        }
    }
}

void insereEstatistica(int pidExecutor, processo *aux, int pid, int exec) {
    for(int i=0; i<numProcessos; i++) {
        if(listaEst[i].ocupado == 0) {
            listaEst[i].idProcessoExec = pidExecutor;
            listaEst[i].idDonoOriginal = aux->donoProcesso;
            listaEst[i].id = aux->id;
            strcpy(listaEst[i].nome, aux->nome);
            listaEst[i].pid = pid;
            listaEst[i].tempo = 0;
            listaEst[i].status = -1;
            listaEst[i].exec = exec;    // 1 == meu processo, 3 == roubado
            listaEst[i].ocupado = 1;
            break;
        }
    }
}

void striped(int *stripedFlag, processo *sharedListProcessos, int processoAux) {
    
    while (stripedFlag[0] == -1) {

        if (stripedFlag[2] > numProcessos -1) {   // Se o contador de processos for maior ou igual ultimo processo da lista, ele finaliza o while principal,
            stripedFlag[0] = 1;                     // ou seja, terminou a distribuição
            stripedFlag[3] = 1; 
            break;
        }

        while(stripedFlag[1] != processoAux && stripedFlag[3] == 0);
                                                // Espera ate ser a sua vez de se atribuir um processo, no caso ate striped[1] ser Zero.
                                                // Importante ressaltar que o processo Aux 0 nunca passa daqui até ser sua vez novamente,
                                                // ou seja, ate que o processo Aux 3 diga que é sua vez "striped[1] = 0;"
        
        stripedFlag[2] = atribuiProcesso(stripedFlag[2], sharedListProcessos);
        
        if (processoAux != PROCESSOS_AUX - 1) {     // Se não estivermos no ultimo processo auxiliar
            stripedFlag[1] = processoAux + 1;       // Define o numero do proximo processo auxiliar a se atribuir um processo
        } else {
            stripedFlag[1] = 0;                     // Estamos no ultimo processo auxiliar, entao voltamos ao processo auxiliar 0
        }
    }
    // Mesma lógica para o restante do processos auxiliares.
}