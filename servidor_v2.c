/****************************************************************************************
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 ** Este módulo permite a inicialização do servidor, a criação e gestão de servidores dedicados, a comunicação com os clientes via IPC (message queue, semáforos e shared memory),
 * o tratamento de sinais (nomeadamente SIGINT, SIGCHLD, SIGUSR2), e a escrita de logs de entrada/saída das viaturas no ficheiros estacionamentos.txt
 * Para além disso garante também o encerramento controlado do sistema, libertando os recursos IPC e coordenando a terminação dos processos filhos.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int nrServidoresDedicados = 0;          // Número de servidores dedicados (só faz sentido no processo Servidor)
int shmId = -1;                         // Variável que tem o ID da Shared Memory
int msgId = -1;                         // Variável que tem o ID da Message Queue
int semId = -1;                         // Variável que tem o ID do Grupo de Semáforos
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento = NULL;   // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD = -1;                // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile = -1;               // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
int shmIdFACE = -1;                     // Variável que tem o ID da Shared Memory da entidade externa FACE
int semIdFACE = -1;                     // Variável que tem o ID do Grupo de Semáforos da entidade externa FACE
int *tarifaAtual = NULL;                // Inteiro definido pela entidade externa FACE com a tarifa atual do parque

/**
 * @brief  Processamento do processo Servidor e dos processos Servidor Dedicado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @return Success (0) or not (<> 0)
 */
int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_ArmaSinaisServidor();
    s1_3_CriaMsgQueue(IPC_KEY, &msgId);
    s1_4_CriaGrupoSemaforos(IPC_KEY, &semId);
    s1_5_CriaBD(IPC_KEY, &shmId, dimensaoMaximaParque, &lugaresEstacionamento);

    so_debug(">");
}

/**
 * @brief s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 * @param pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if (argc != 2) {
        so_error("S1.1", "Número de argumentos inválido. Uso: ./servidor <dimensaoParque>");
        exit(EXIT_FAILURE);
    }

    char *endptr;
    long valor = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0' || valor <= 0) {
        so_error("S1.1", "Valor inválido para dimensão do parque: %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    *pdimensaoMaximaParque = (int)valor;
    so_success("S1.1", "Parque com dimensão %d", *pdimensaoMaximaParque);

    so_debug("> [@return *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief s1_2_ArmaSinaisServidor Ler a descrição da tarefa S1.2 no enunciado
 */
void s1_2_ArmaSinaisServidor() {
    so_debug("<");

    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar SIGINT");
        exit(1);
    }
    // Arma sinal SIGCHLD 
    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar SIGCHLD");
        exit(1);
    }

    so_success("S1.2", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief s1_3_CriaMsgQueue Ler a descrição da tarefa s1.3 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    // Tenta obter a fila 
    int existente = msgget(ipcKey, 0666);
    if (existente != -1) {
        // Se já existir, tenta remover
        if (msgctl(existente, IPC_RMID, NULL) == -1) {
            so_error("S1.3", "Erro ao remover fila de mensagens antiga");
            exit(1); 
        }
        // Tenta criar nova fila 
        *pmsgId = msgget(ipcKey, IPC_CREAT | IPC_EXCL | 0666);
        if (*pmsgId == -1) {
            so_error("S1.3", "Erro ao criar nova fila de mensagens após remoção");
            exit(1); 
        }
    } else {
        // Se a fila ainda nao existia, tenta criar
        *pmsgId = msgget(ipcKey, IPC_CREAT | IPC_EXCL | 0666);
        if (*pmsgId == -1) {
            so_error("S1.3", "Erro ao criar nova fila de mensagens");
            exit(1);  
        }
    }

    so_success("S1.3", "Fila de mensagens criada com sucesso");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief s1_4_CriaGrupoSemaforos Ler a descrição da tarefa s1.4 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param psemId (O) identificador aberto de IPC
 */
 void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    int semExistente = semget(ipcKey, 0, 0666);  // Verifica se já existe

    if (semExistente != -1) {
        // Tenta remover semáforos antigos
        if (semctl(semExistente, 0, IPC_RMID) == -1) {
            so_error("S1.4", "Erro ao remover grupo de semáforos existente");
            exit(1); 
        }
    }

    // Cria novo grupo com 4 semáforos
    *psemId = semget(ipcKey, 4, IPC_CREAT | IPC_EXCL | 0666);
    if (*psemId == -1) {
        so_error("S1.4", "Erro ao criar grupo de semáforos");
        return;  
    }
    if (semctl(*psemId, SEM_MUTEX_BD, SETVAL, 1) == -1 ||
        semctl(*psemId, SEM_MUTEX_LOGFILE, SETVAL, 1) == -1 ||
        semctl(*psemId, SEM_SRV_DEDICADOS, SETVAL, 0) == -1 ||
        semctl(*psemId, SEM_LUGARES_PARQUE, SETVAL, dimensaoMaximaParque) == -1) {
        so_error("S1.4", "Erro ao inicializar semáforos");
        return; 
    }

    so_success("S1.4", "Grupo de semáforos criado e inicializado com sucesso");
    so_debug("> [@return *psemId:%d]", *psemId);
}
/**
 * @brief s1_5_CriaBD Ler a descrição da tarefa S1.5 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pshmId (O) identificador aberto de IPC
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param ipcKey:0x0%x, dimensaoMaximaParque:%d]", ipcKey, dimensaoMaximaParque);

    size_t shm_size = dimensaoMaximaParque * sizeof(Estacionamento);
    int nova = 0;

    // Tenta ligar-se à SHM existente
    *pshmId = shmget(ipcKey, shm_size, 0666);
    if (*pshmId == -1) {
        if (errno == ENOENT) {
            // Se a SHM não existe, tenta criar
            *pshmId = shmget(ipcKey, shm_size, IPC_CREAT | 0666);
            if (*pshmId == -1) {
                so_error("S1.5", "Erro ao criar SHM");
                exit(1);
            }
            nova = 1;
        } else {
            so_error("S1.5", "Erro ao obter SHM existente");
            exit(1);
        }
    }

    *plugaresEstacionamento = (Estacionamento *)shmat(*pshmId, NULL, 0);
    if (*plugaresEstacionamento == (void *)-1) {
        so_error("S1.5", "Erro ao ligar à SHM");
        exit(1);
    }

    if (nova) {
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
            (*plugaresEstacionamento)[i].pidServidorDedicado = -1;
            memset(&(*plugaresEstacionamento)[i].viatura, 0, sizeof(Viatura));
        }
    }

    so_success("S1.5", "SHM pronta");
    so_debug("> [@return *pshmId:%d, *plugaresEstacionamento:%p]", *pshmId, *plugaresEstacionamento);
}

/**
 * @brief s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s2_MainServidor() {
    so_debug("<");

    while (TRUE) {
        s2_1_LePedidoCliente(msgId, &clientRequest);
        s2_2_CriaServidorDedicado(&nrServidoresDedicados);
    }

    so_debug(">");
}

/**
 * @brief s2_1_LePedidoCliente Ler a descrição da tarefa S2.1 no enunciado.
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    ssize_t recebido = msgrcv(msgId, pclientRequest, sizeof(MsgContent) - sizeof(long), MSGTYPE_LOGIN, 0);
    if (recebido == -1) {
        so_error("S2.1", "Erro ao receber mensagem do cliente");
        s4_EncerraServidor();
        return;
    }

    if (pclientRequest->msgType != MSGTYPE_LOGIN) {
        so_error("S2.1", "Mensagem recebida com tipo inválido: %ld", pclientRequest->msgType);
        s4_EncerraServidor();
        return;
    }

    so_success("S2.1", "%s %d",
        pclientRequest->msgData.est.viatura.matricula,
        pclientRequest->msgData.est.pidCliente);

    sleep(10);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
                // depois de terem a certeza que não terão uma espera ativa

    so_debug("> [@return *pclientRequest:[%s:%s:%c:%s:%d.%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief s2_2_CriaServidorDedicado Ler a descrição da tarefa S2.2 no enunciado
 * @param pnrServidoresDedicados (O) número de Servidores Dedicados que foram criados até então
 */
void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados) {
    so_debug("<");

    pid_t pid = fork();

    if (pid < 0) {
        so_error("S2.2", "Erro ao criar Servidor Dedicado");
        s4_EncerraServidor();
        return;
    }

    if (pid == 0) {
        clientRequest.msgData.est.pidServidorDedicado = getpid();
        so_success("S2.2", "SD: Nasci com PID %d", getpid());
        sd7_1_ArmaSinaisServidorDedicado();
        sd7_MainServidorDedicado();
        
        exit(0); 
    } else {
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            if (lugaresEstacionamento[i].pidCliente == clientRequest.msgData.est.pidCliente) {
                lugaresEstacionamento[i].pidServidorDedicado = pid;
                break;
            }
        }
        (*pnrServidoresDedicados)++;
        so_success("S2.2", "Servidor: Iniciei SD %d", pid);
    }
    so_debug("> [@return *pnrServidoresDedicados:%d", *pnrServidoresDedicados);
}

/**
 * @brief s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");

    s4_EncerraServidor();
    so_debug(">");
}

/**
 * @brief s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s4_EncerraServidor() {
    so_debug("<");

    s4_1_TerminaServidoresDedicados(lugaresEstacionamento, dimensaoMaximaParque);
    s4_2_AguardaFimServidoresDedicados(nrServidoresDedicados);
    s4_3_ApagaElementosIPCeTermina(shmId, semId, msgId);

    so_debug(">");
}

/**
 * @brief s4_1_TerminaServidoresDedicados Ler a descrição da tarefa S4.1 no enunciado
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 */
void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque) {
    so_debug("< [@param lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", lugaresEstacionamento, dimensaoMaximaParque);

    struct sembuf op;

    op.sem_num = SEM_MUTEX_BD;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semId, &op, 1) == -1) {
        so_error("S4.1", "Erro ao bloquear semáforo MUTEX_BD");
        return;
    }
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        int pidCliente = lugaresEstacionamento[i].pidCliente;
        int pidSD = lugaresEstacionamento[i].pidServidorDedicado;

        if (pidCliente != DISPONIVEL && pidSD > 0) {
            if (kill(pidSD, SIGUSR2) == 0) {
                so_success("S4.1", "Enviei SIGUSR2 a SD %d", pidSD);
            } else {
                so_error("S4.1", "Falha ao enviar SIGUSR2 a SD %d", pidSD);
            }
        }
    }
    op.sem_op = 1;
    if (semop(semId, &op, 1) == -1) {
        so_error("S4.1", "Erro ao libertar semáforo MUTEX_BD");
        return;
    }
    so_debug(">");
}

/**
 * @brief s4_2_AguardaFimServidoresDedicados Ler a descrição da tarefa S4.2 no enunciado
 * @param nrServidoresDedicados (I) número de Servidores Dedicados que foram criados até então
 */
void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados) {
    so_debug("< [@param nrServidoresDedicados:%d]", nrServidoresDedicados);

    struct sembuf op = {SEM_SRV_DEDICADOS, -nrServidoresDedicados, 0};

    if (semop(semId, &op, 1) == -1) {
        so_error("S4.2", "Erro ao aguardar fim dos SDs");
        so_debug(">");
        return;  
    }

    so_success("S4.2", "Todos os SDs terminaram");
    so_debug(">");
}

/**
 * @brief s4_3_ApagaElementosIPCeTermina Ler a descrição da tarefa S4.2 no enunciado
 * @param shmId (I) identificador aberto de IPC
 * @param semId (I) identificador aberto de IPC
 * @param msgId (I) identificador aberto de IPC
 */
void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId) {
    so_debug("< [@param shmId:%d, semId:%d, msgId:%d]", shmId, semId, msgId);

    shmctl(shmId, IPC_RMID, NULL);
    semctl(semId, 0, IPC_RMID);
    msgctl(msgId, IPC_RMID, NULL);
    so_success("S4.3", "Servidor: End Shutdown");
    exit(0);
    so_debug(">");
}

/**
 * @brief s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    int status;
    pid_t pid;
    
    pid = wait(&status);
    
    if (pid > 0) {
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pid);
        
        nrServidoresDedicados--;
        
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            if (lugaresEstacionamento[i].pidServidorDedicado == pid) {

                memset(&lugaresEstacionamento[i].viatura, 0, sizeof(lugaresEstacionamento[i].viatura));
                lugaresEstacionamento[i].viatura.categoria = '-';
                lugaresEstacionamento[i].pidCliente = -1;
                lugaresEstacionamento[i].pidServidorDedicado = -1;
                break;
            }
        }
    }

    so_debug("> [@return nrServidoresDedicados:%d]", nrServidoresDedicados);
}
/**
 * @brief sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_GetShmFACE(KEY_FACE, &shmIdFACE);
    sd7_4_GetSemFACE(KEY_FACE, &semIdFACE);
    sd7_5_ProcuraLugarDisponivelBD(semId, clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSucessoAoCliente(msgId, clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout(msgId);
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao ignorar CTRL+C");
        exit(EXIT_FAILURE); 
    }

    if (signal(SIGALRM, sd10_1_1_TrataAlarme) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar SIGALRM");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar SIGUSR2");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.1", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if (clientRequest.msgData.est.pidCliente > 0) {
        so_success("SD7.2", "");
    } else {
        so_error("SD7.2", "PID do cliente inválido");
        exit(EXIT_FAILURE);
    }
    so_debug(">");
}

/**
 * @brief sd7_3_GetShmFACE Ler a descrição da tarefa SD7.3 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param pshmIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    *pshmIdFACE = shmget(ipcKeyFace, 0, 0666);
    if (*pshmIdFACE == -1) {
        so_error("SD7.3", "Erro ao obter SHM da FACE");
        exit(EXIT_FAILURE);
    }

    tarifaAtual = (int *)shmat(*pshmIdFACE, NULL, 0);
    if (tarifaAtual == (void *)-1) {
        so_error("SD7.3", "Erro ao ligar à SHM da FACE");
        exit(EXIT_FAILURE);
    }
    so_success("SD7.3", "");
    so_debug("> [@return *pshmIdFACE:%d]", *pshmIdFACE);
}

/**
 * @brief sd7_4_GetSemFACE Ler a descrição da tarefa SD7.4 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param psemIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    *psemIdFACE = semget(ipcKeyFace, 0, 0666);
    if (*psemIdFACE == -1) {
        so_error("SD7.4", "Erro ao obter grupo de semáforos da FACE");
        exit(EXIT_FAILURE);
    }
    so_success("SD7.4", "");
    so_debug("> [@return *psemIdFACE:%d]", *psemIdFACE);
}

/**
 * @brief sd7_5_ProcuraLugarDisponivelBD Ler a descrição da tarefa SD7.5 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param semId:%d, clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", semId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    struct sembuf down = {SEM_LUGARES_PARQUE, -1, 0};
    semop(semId, &down, 1);

    struct sembuf mutexDown = {SEM_MUTEX_BD, -1, 0};
    struct sembuf mutexUp = {SEM_MUTEX_BD, 1, 0};
    semop(semId, &mutexDown, 1);

    int indexEncontrado = -1;

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            
            lugaresEstacionamento[i] = clientRequest.msgData.est;
            indexEncontrado = i;
            *pindexClienteBD = i;
            break;
        }
    }

    semop(semId, &mutexUp, 1);

    if (indexEncontrado != -1) {
        so_success("SD7.5", "Reservei Lugar: %d", indexEncontrado);
    } else {
        so_error("SD7.5", "Não foi encontrado lugar (deveria ser impossível)");
        exit(EXIT_FAILURE);
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(MsgContent clientRequest) {
    #include <regex.h>
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    regex_t regex;
    if (regcomp(&regex, "^[A-Z0-9]+$", REG_EXTENDED) != 0) {
        so_error("SD8.1", "Erro ao compilar regex");
        exit(1);
    }

    if (regexec(&regex, clientRequest.msgData.est.viatura.matricula, 0, NULL, 0) == 0) {
        so_success("SD8.1", "");
    } else {
        so_error("SD8.1", "Matrícula inválida: %s", clientRequest.msgData.est.viatura.matricula);
        sd11_EncerraServidorDedicado();
    }

    regfree(&regex);
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    char *pais = clientRequest.msgData.est.viatura.pais;
    if (strlen(pais) == 2 && isupper(pais[0]) && isupper(pais[1])) {
        so_success("SD8.2", "");
    } else {
        so_error("SD8.2", "País inválido: %s", pais);
        sd11_EncerraServidorDedicado();
    }
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    char cat = clientRequest.msgData.est.viatura.categoria;
    if (cat == 'P' || cat == 'L' || cat == 'M') {
        so_success("SD8.3", "");
    } else {
        so_error("SD8.3", "Categoria inválida: %c", cat);
        sd11_EncerraServidorDedicado();
    }
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(MsgContent clientRequest) {
    
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if (strcmp(clientRequest.msgData.est.viatura.nomeCondutor, "Nome Inexistente") == 0) {
        so_error("SD8.4", "Nome de utilizador inválido: %s", clientRequest.msgData.est.viatura.nomeCondutor);
        sd11_EncerraServidorDedicado();  
    } else {
        so_success("SD8.4", "Utilizador válido: %s", clientRequest.msgData.est.viatura.nomeCondutor);
    }
    so_debug(">");
}

/**
 * @brief sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    srand(getpid());
    int tempo = rand() % MAX_ESPERA + 1;
    sleep(tempo);
    so_success("SD9.1", "%d", tempo);
    so_debug(">");
}

/**
 * @brief sd9_2_EnviaSucessoAoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    MsgContent resposta = clientRequest;
    resposta.msgData.status = CLIENT_ACCEPTED;

    if (msgsnd(msgId, &resposta, sizeof(resposta.msgData), 0) == -1) {
        so_error("SD9.2", "Erro ao enviar confirmação ao cliente");
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);

    so_debug(">");
}

/**
 * @brief sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    FILE *fp = fopen(logFilename, "ab+");
    if (!fp) {
        so_error("SD9.3", "Erro ao abrir ficheiro de log");
        sd11_EncerraServidorDedicado();
        return;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        so_error("SD9.3", "Erro ao posicionar no fim do ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
    *pposicaoLogfile = ftell(fp);
    if (*pposicaoLogfile == -1L) {
        so_error("SD9.3", "Erro ao obter posição no ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    // Formato para a data de entrada: "YYYY-MM-DDTHHhMM"
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", tm_info);
    strcpy(plogItem->dataSaida, ""); 
    plogItem->viatura = clientRequest.msgData.est.viatura;

    if (fwrite(plogItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD9.3", "Erro ao escrever no ficheiro de log");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
    fclose(fp);

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",
        *pposicaoLogfile,
        plogItem->viatura.matricula,
        plogItem->dataEntrada
    );
    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 */
void sd10_1_AguardaCheckout(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);

     MsgContent mensagem;
    long tipo = (long)getpid();

    alarm(60); // Evita bloqueio permanente

    for (int i = 0; i < 2; i++) {
        ssize_t bytes = msgrcv(msgId, &mensagem, sizeof(mensagem.msgData), tipo, 0);

        if (bytes == -1) {
            if (errno == EINTR)
                continue;

            so_error("SD10.1", "Erro ao ler mensagem de checkout: %s", strerror(errno));
            sd11_EncerraServidorDedicado();
            break;
        }

        if (mensagem.msgData.status == TERMINA_ESTACIONAMENTO) {
            clientRequest.msgData = mensagem.msgData;
            break;  
        }
    }

    if (clientRequest.msgData.status == TERMINA_ESTACIONAMENTO) {
        so_success("SD10.1", "SD: A viatura %s deseja sair do parque",
                   clientRequest.msgData.est.viatura.matricula);
    }

    so_debug(">");
}

/**
 * @brief  sd10_1_1_TrataAlarme Ler a descrição da tarefa SD10.1.1 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd10_1_1_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    MsgContent infoMsg = clientRequest;
    infoMsg.msgType = clientRequest.msgData.est.pidCliente;
    infoMsg.msgData.status = INFO_TARIFA;

    struct sembuf down = {0, -1, IPC_NOWAIT};
    struct sembuf up = {0, 1, 0};

    int tarifa = -1;

    if (semop(semIdFACE, &down, 1) == 0) {
        tarifa = *tarifaAtual;
        semop(semIdFACE, &up, 1);
    } else {
        so_error("SD10.1.1", "Semáforo ocupado, ignoro leitura");
        return;
    }

    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%Hh%M", tm_info);

    snprintf(infoMsg.msgData.infoTarifa, sizeof(infoMsg.msgData.infoTarifa),
             "%s Tarifa atual:%d", timestamp, tarifa);

    if (msgsnd(msgId, &infoMsg, sizeof(infoMsg.msgData), 0) == -1) {
        so_error("SD10.1.1", "Erro ao enviar mensagem INFO_TARIFA");
        return;
    }
    so_success("SD10.1.1", "Info Tarifa");
    alarm(60);
    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]", logFilename, posicaoLogfile, logItem.viatura.matricula, logItem.viatura.pais, logItem.viatura.categoria, logItem.viatura.nomeCondutor, logItem.dataEntrada, logItem.dataSaida);

    FILE *fp = fopen(logFilename, "rb+");
    if (!fp) {
        so_error("SD10.2", "Erro ao abrir ficheiro de log para escrita");
        sd11_EncerraServidorDedicado();
        return;
    }
    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {
        so_error("SD10.2", "Erro ao posicionar ficheiro no offset");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", tm_info);

    if (fwrite(&logItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD10.2", "Erro ao escrever dados no ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        return;
    }
    fclose(fp);

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
        posicaoLogfile,
        logItem.viatura.matricula,
        logItem.dataSaida
    );
    so_debug(">");
    sd11_EncerraServidorDedicado();
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(semId, lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaTerminarAoClienteETermina(msgId, clientRequest);

    so_debug(">");
}

/**
 * @brief sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param semId:%d, lugaresEstacionamento:%p, indexClienteBD:%d]", semId, lugaresEstacionamento, indexClienteBD);

    struct sembuf down = {SEM_MUTEX_BD, -1, 0};
    struct sembuf up = {SEM_MUTEX_BD, 1, 0};

    semop(semId, &down, 1);
    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = -1;
    semop(semId, &up, 1);

    struct sembuf signalLugar = {SEM_LUGARES_PARQUE, 1, 0};
    semop(semId, &signalLugar, 1);

    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief sd11_2_EnviaTerminarAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    MsgContent resposta = clientRequest;
    resposta.msgData.status = ESTACIONAMENTO_TERMINADO;

    if (msgsnd(msgId, &resposta, sizeof(resposta.msgData), 0) == -1) {
        so_error("SD11.2", "Erro ao enviar fim ao cliente");
        exit(1);
    } else {
        so_success("SD11.2", "SD: Shutdown");
        exit(0);
    }
    struct sembuf signal = {SEM_SRV_DEDICADOS, 1, 0};
    semop(semId, &signal, 1);

    so_debug(">");
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    struct sembuf op_barreira = {SEM_SRV_DEDICADOS, 1, 0};
    if (semop(semId, &op_barreira, 1) == -1) {
        so_error("SD12", "Erro ao incrementar barreira");
    }
    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
    sd11_EncerraServidorDedicado();

    so_debug(">");
}
