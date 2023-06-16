#include "funcs.h"

void limpeza() {
    printf("\nLimpando.....\n");
    int idtest = -2;
     if ( (idtest = semget(0x1223, 1, IPC_EXCL | IPC_CREAT | 0x1ff) ) == -1) {
        if (errno == EEXIST) {
            if (semctl(idsem, 0, IPC_RMID, operacao) < 0) {
                printf("Erro ao destruir Semaforos...\n");
            } else {
                // printf("Sucesso..1 (EXIST)\n");
            }
        } else {
            printf("Erro ao criar/obter o semáforo\n");
        }
    } else {
        //printf("idtest: %d\n", idtest);
        if (semctl(idtest, 0, IPC_RMID, operacao) < 0) {
            printf("Erro ao destruir Semaforo TESTE...\n");
        } else {
            // printf("Sucesso..1 (TESTE)\n");
        }
    }
    
    
    if(shmdt(stripedFlag) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..2\n");
    }

    if(shmdt(sharedListProcessos) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..3\n");
    }

    if(shmdt(tempoTotal) < 0) {
        printf("Erro ao desanexar segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..4\n");
    }

    if(shmctl(idMemoriaStripedVetor, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..5\n");
    }

    if(shmctl(idMemoriaProcessos, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..6\n");
    }

    if(shmctl(idMemoriaTempo, IPC_RMID, NULL) < 0) {
        printf("Erro ao DESTRUIR segmento de memoria compartilhada...\n");
    } else {
        // printf("Sucesso..7\n");
    }

    exit(0);
}

int p_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = 0;
    operacao[0].sem_flg = 0;

    operacao[1].sem_num = 0;
    operacao[1].sem_op = 1;
    operacao[1].sem_flg = 0;
    if (semop(idsem, operacao, 2) < 0) {
        printf("erro no p=%d\n", errno);
    }
}

int v_sem() {
    operacao[0].sem_num = 0;
    operacao[0].sem_op = -1;
    operacao[0].sem_flg = 0;
    if (semop(idsem, operacao, 1) < 0) {
        printf("erro no p=%d\n", errno);
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

void printProcessos(processo *lista) {
    while (lista) {
        printf("\nId: %d\n", lista->id);
        printf("Nome: %s\n", lista->nome);
        printf("Estado: %d\n", lista->estado);
        printf("Dono Processo: %d\n", lista->donoProcesso);
        
        lista = lista->prox;
    }
}

processo* buscaProcesso(processo *lista, pid_t pid) {
    int achou = 0;
    
    while(lista) {
        //printf("dono: %d || estado: %d\n", lista->donoProcesso, lista->estado);
        if(lista->donoProcesso == pid && lista->estado == 0) {
            achou = 1;
            //printf("Achou dono: %d || estado: %d\n", lista->donoProcesso, lista->estado);
            break;
        }
        lista = lista->prox;
    }

    if(achou == 1) {
        return lista;
    }
    return NULL;
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
            
            //printf("id:%d\n", lista->id);
            lista = lista->prox;
        }
    }
}

void liberaListaProcessos(processo **lista) {
    while(*lista) {
        processo *aux = (*lista)->prox;
        free(*lista);
        *lista = aux;
    }
}

int atribuiProcesso(int striped_dois, processo *sharedListProcessos, int numProcessos) {
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

int leArquivo(processo **lista, char *nomeArquivo) {
    char *read = malloc(sizeof(char) * 20);
    FILE *arqProcessos;

    arqProcessos = fopen(nomeArquivo, "r");

    if (arqProcessos == NULL) {
        printf("ERRO! O arquivo de processos não foi aberto!\n");
        exit(1);
    }
    
    int id = 0;
    while(fgets(read, 20, arqProcessos) != NULL) {
        int tam = strlen(read);
        read[tam-1] = '\0';
        // printf("read: %s - %ld\n", read, strlen(read));
        if(strlen(read) > 0) {
            insereProcesso(lista, id, read);
            id++;
        }
    }
    fclose(arqProcessos);

    return id;
}



void striped(int *stripedFlag, processo *sharedListProcessos, int numProcessos, int processoAux) {
    
    while (stripedFlag[0] == -1) {
            
        //
        if (stripedFlag[2] > numProcessos -1) {   // Se o contador de processos for maior ou igual ultimo processo da lista, ele finaliza o while principal,
            stripedFlag[0] = 1;                     // ou seja, terminou a distribuição
            // printf("\nFIM: Sou filho(%d) pid:%d    PAI(%d) || Contador: %d || Distrib: %d\n", processoAux, getpid(), getppid(), stripedFlag[2], stripedFlag[0]);
            // printf("FIMMMMMMM %d\n", processoAux);
            stripedFlag[3] = 1; 
            break;
        }

        //printf("\nEntrou striped.. %d || Vez do AUX: %d || contador: %d - %d\n", processoAux, stripedFlag[1], stripedFlag[2], stripedFlag[3]);
        
        while(stripedFlag[1] != processoAux && stripedFlag[3] == 0) {
            // if(stripedFlag[3] == 1) {
            //     break;
            // }
            // printf("to rodando....%d - f: %d - %d\n", processoAux, stripedFlag[1], stripedFlag[3]);
        }
                                                // Espera ate ser a sua vez de se atribuir um processo, no caso ate striped[1] ser Zero.
                                                // Importante ressaltar que o processo Aux 0 nunca passa daqui até ser sua vez novamente,
                                                // ou seja, ate que o processo Aux 3 diga que é sua vez "striped[1] = 0;"
        //printf("Passou beasy wait.. %d || minha vez: %d\n", processoAux, stripedFlag[1]);
        
        stripedFlag[2] = atribuiProcesso(stripedFlag[2], sharedListProcessos, numProcessos);

        
        if (processoAux != PROCESSOS_AUX - 1) {     // Se não estivermos no ultimo processo auxiliar
            stripedFlag[1] = processoAux + 1;       // Define o numero do proximo processo auxiliar a se atribuir um processo
        } else {
            stripedFlag[1] = 0;                     // Estamos no ultimo processo auxiliar, entao voltamos ao processo auxiliar 0
        }
        


        //printf("proximo: %d | Contador: %d || filho %d\n",stripedFlag[1], stripedFlag[2], processoAux);
    }
    // Mesma lógica para o restante do processos auxiliares.
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