/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
 **
 ** Aluno:Inês Lage Marchante Nº: 129846      Nome: Inês Marchante
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **Este módulo tem como objetivo principal a representação do servidor que coordena o funcionamento do parque de estacionamento
 **Recebe desde pedidos dos clientes via FIFO, valida os pedidos, cria processos filhos (servidores dedicados), regista a entrada e saida em ficheiro das viaturas, e gere os sinais do sistema
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento;  // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD;                     // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile;                    // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile

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
}

/**
 * @brief  s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_CriaBD(dimensaoMaximaParque, &lugaresEstacionamento);
    s1_3_ArmaSinaisServidor();
    s1_4_CriaFifoServidor(FILE_REQUESTS);

    so_debug(">");
}

/**
 * @brief  s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @param  pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if (argc != 2) {
        so_error("S1.1", "Número inválido de argumentos. Uso: ./servidor <dimensão>");
        exit(1);  
    }

    *pdimensaoMaximaParque = atoi(argv[1]);

    if (*pdimensaoMaximaParque <= 0) {
        so_error("S1.1", "Dimensão inválida do parque (<= 0)");
        exit(1);
    }
    //Lê argv e guarda o número de lugares
    so_success("S1.1", "Dimensão do parque: %d", *pdimensaoMaximaParque);

    so_debug("> [@param +pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief  s1_2_CriaBD Ler a descrição da tarefa S1.2 no enunciado
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param dimensaoMaximaParque:%d]", dimensaoMaximaParque);

    *plugaresEstacionamento = (Estacionamento *) malloc(dimensaoMaximaParque * sizeof(Estacionamento));

    if (*plugaresEstacionamento == NULL) {
        so_error("S1.2", "Erro ao alocar memória para a BD");
        exit(1);
    }

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        strcpy((*plugaresEstacionamento)[i].viatura.matricula, "");
        strcpy((*plugaresEstacionamento)[i].viatura.pais, "");
        (*plugaresEstacionamento)[i].viatura.categoria = '-';
        strcpy((*plugaresEstacionamento)[i].viatura.nomeCondutor, "");
        (*plugaresEstacionamento)[i].pidCliente = -1;
        (*plugaresEstacionamento)[i].pidServidorDedicado = -1;
    }

    so_success("S1.2", "Base de dados criada com %d lugares", dimensaoMaximaParque);

    so_debug("> [*plugaresEstacionamento:%p]", *plugaresEstacionamento);
}

/**
 * @brief  s1_3_ArmaSinaisServidor Ler a descrição da tarefa S1.3 no enunciado
 */
void s1_3_ArmaSinaisServidor() {
    so_debug("<");
//Configura os sinais SIGINT e SIGCHLD
    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.3", "Erro ao armar sinal SIGINT");
        exit(1);
    }

    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.3", "Erro ao armar sinal SIGCHLD");
        exit(1);
    }

    so_success("S1.3", "Sinais SIGINT e SIGCHLD armados");

    so_debug(">");
}

/**
 * @brief  s1_4_CriaFifoServidor Ler a descrição da tarefa S1.4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void s1_4_CriaFifoServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);
//Cria FIFO (server.fifo) para comunicação com os clientes
    unlink(filenameFifoServidor);  // apaga FIFO antigo, se existir

    if (mkfifo(filenameFifoServidor, 0666) == -1) {
        so_error("S1.4", "Erro ao criar FIFO com mkfifo()");
        exit(1);
    }

    so_success("S1.4", "FIFO '%s' criado com sucesso", filenameFifoServidor);

    so_debug(">");
}

/**
 * @brief  s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO, exceto depois de
 *         realizada a função s2_1_AbreFifoServidor(), altura em que podem
 *         comentar o statement sleep abaixo (que, neste momento está aqui
 *         para evitar que os alunos tenham uma espera ativa no seu código)
 */
void s2_MainServidor() {
    so_debug("<");

    FILE *fFifoServidor;
    while (TRUE) { 
        s2_1_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
        s2_2_LePedidosFifoServidor(fFifoServidor);
        //sleep(10);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
                    // depois de terem a certeza que não terão uma espera ativa
    }
    so_debug(">");
}

/**
 * @brief  s2_1_AbreFifoServidor Ler a descrição da tarefa S2.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    while (TRUE) {
        *pfFifoServidor = fopen(filenameFifoServidor, "rb");

        if (*pfFifoServidor != NULL) {
            break; 
        }

        if (errno == EINTR) {
            continue; // foi interrompido por sinal, tenta de novo
        }

        so_error("S2.1", "Erro ao abrir FIFO com fopen()");
        s4_EncerraServidor(filenameFifoServidor);
        exit(1);
    }

    so_success("S2.1", "FIFO aberto para leitura");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  s2_2_LePedidosFifoServidor Ler a descrição da tarefa S2.2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_2_LePedidosFifoServidor(FILE *fFifoServidor) {
    so_debug("<");

    int terminaCiclo2 = FALSE;
    while (TRUE) {
        terminaCiclo2 = s2_2_1_LePedido(fFifoServidor, &clientRequest);
        if (terminaCiclo2)
            break;
        s2_2_2_ProcuraLugarDisponivelBD(clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
        s2_2_3_CriaServidorDedicado(lugaresEstacionamento, indexClienteBD);
    }

    so_debug(">");
}

/**
 * @brief  s2_2_1_LePedido Ler a descrição da tarefa S2.2.1 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  pclientRequest (O) pedido recebido, enviado por um Cliente
 * @return TRUE se não conseguiu ler um pedido porque o FIFO não tem mais pedidos.
 */
int s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest) {
    int naoHaMaisPedidos = TRUE;
    so_debug("< [@param fFifoServidor:%p]", fFifoServidor);
   
    size_t bytesLidos = fread(pclientRequest, sizeof(Estacionamento), 1, fFifoServidor);

    if (bytesLidos == 1) {
        naoHaMaisPedidos = FALSE;
        so_success("S2.2.1", "Li Pedido do FIFO");
    } else if (feof(fFifoServidor)) {
        
        so_success("S2.2.1", "Não há mais registos no FIFO");
        
    } else if (ferror(fFifoServidor)) {
        if (errno == EINTR) {
            clearerr(fFifoServidor);
            // Continua a tentar
        } else {
            so_error("S2.2.1", "Erro na leitura do FIFO");
            fclose(fFifoServidor);
            s4_EncerraServidor(FILE_REQUESTS);
            naoHaMaisPedidos= TRUE;
        }
    }
    so_debug("> [naoHaMaisPedidos:%d, *pclientRequest:[%s:%s:%c:%s:%d.%d]]", naoHaMaisPedidos,
             pclientRequest->viatura.matricula, pclientRequest->viatura.pais,
             pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor,
             pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);

    return naoHaMaisPedidos;
}

/**
 * @brief  s2_2_2_ProcuraLugarDisponivelBD Ler a descrição da tarefa S2.2.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado, lugaresEstacionamento, dimensaoMaximaParque);

    *pindexClienteBD = -1;

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        
        if (lugaresEstacionamento[i].pidCliente == -1) {
            lugaresEstacionamento[i] = clientRequest;
            *pindexClienteBD = i;
            so_success("S2.2.2", "Reservei Lugar:", i);
            break;
        }
    }

    if (*pindexClienteBD == -1) {
        so_error("S2.2.2", "Parque cheio");
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  s2_2_3_CriaServidorDedicado    Ler a descrição da tarefa S2.2.3 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD < 0) {
        so_error("S2.2.3", "Índice inválido (parque cheio)");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        so_error("S2.2.3", "Erro no fork()");
        s4_EncerraServidor(FILE_REQUESTS);
        exit(1);
        return;
    }

    if (pid == 0) {
        // Processo filho
        so_success("S2.2.3", "SD: Nasci com PID", getpid());
        clientRequest = lugaresEstacionamento[indexClienteBD];
        indexClienteBD = indexClienteBD; 
        sd7_MainServidorDedicado();
        exit(0);
    }

    lugaresEstacionamento[indexClienteBD].pidServidorDedicado = pid;

    so_success("S2.2.3", "Servidor: Iniciei SD %d", pid);
    so_debug(">");
}

/**
 * @brief  s3_TrataCtrlC    Ler a descrição da tarefa S3 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    //Ao receber SIGINT envia SIGUSR2 a todos os filhos ativos e chama S4
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown"); 

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        pid_t pid = lugaresEstacionamento[i].pidServidorDedicado;
        
        if (pid > 1) {  
            int jaEnviei = 0;
            for (int j = 0; j < i; j++) {
                if (lugaresEstacionamento[j].pidServidorDedicado == pid) {
                    jaEnviei = 1;
                    break;
                }
            }
            if (!jaEnviei) {
                if (kill(pid, SIGUSR2) == -1) {
                    so_error("S3", "Erro ao enviar SIGUSR2 ao SD com PID=%d", pid);
                    s4_EncerraServidor(FILE_REQUESTS);
                    exit(1);
                }
            }
        }
    }

    s4_EncerraServidor(FILE_REQUESTS);
    so_debug(">");
    exit(0);
}

/**
 * @brief  s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void s4_EncerraServidor(char *filenameFifoServidor) {
    //remove o FIFO do sistema de ficheiros
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    if (unlink(filenameFifoServidor) == -1) {
        so_error("S4", "Erro ao remover FIFO '%s'", filenameFifoServidor);
        exit(0);
    }
    so_success("S4", "Servidor: End Shutdown"); 
    so_debug(">");
    exit(0);
}

/**
 * @brief  s5_TrataTerminouServidorDedicado    Ler a descrição da tarefa S5 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    //Trata SIGCHLD quando um servidor dedicado termina
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    pid_t pidFilho = wait(NULL); 

    if (pidFilho > 0) {
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pidFilho);

        for (int i = 0; i < dimensaoMaximaParque; i++) {
            if (lugaresEstacionamento[i].pidServidorDedicado == pidFilho) {
                strcpy(lugaresEstacionamento[i].viatura.matricula, "");
                strcpy(lugaresEstacionamento[i].viatura.pais, "");
                lugaresEstacionamento[i].viatura.categoria = '-';
                strcpy(lugaresEstacionamento[i].viatura.nomeCondutor, "");
                lugaresEstacionamento[i].pidCliente = -1;
                lugaresEstacionamento[i].pidServidorDedicado = -1;

                so_success("S5", "Lugar %d libertado pelo Servidor Pai (PID=%d)", i, pidFilho);
                break;
            }
        }
    }

    so_debug(">");
}
/**
 * @brief  sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_ValidaLugarDisponivelBD(indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSigusr1AoCliente(clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout();
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");

    so_debug(">");
}

/**
 * @brief  sd7_1_ArmaSinaisServidorDedicado    Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao ignorar sinal SIGINT");
        exit(1);
    }

    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR2");
        exit(1);
    }
    if (signal(SIGUSR1, sd13_TrataSigusr1) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR1");
        exit(1);
    }

    so_success("SD7.1", "Sinal SIGUSR2 armado no Servidor Dedicado");

    so_debug(">");
}

/**
 * @brief  sd7_2_ValidaPidCliente    Ler a descrição da tarefa SD7.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (clientRequest.pidCliente <= 0) {
        so_error("SD7.2", "PID do cliente inválido");
        exit(1);
    }

    so_success("SD7.2", "PID do cliente é válido: %d", clientRequest.pidCliente);

    so_debug(">");
}

/**
 * @brief  sd7_3_ValidaLugarDisponivelBD    Ler a descrição da tarefa SD7.3 no enunciado
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD) {
    so_debug("< [@param indexClienteBD:%d]", indexClienteBD);

    if (indexClienteBD < 0 || lugaresEstacionamento[indexClienteBD].pidCliente <= 0) {
        so_error("SD7.3", "Lugar de estacionamento inválido ou libertado");
        exit(1);
    }

    so_success("SD7.3", "Lugar de estacionamento válido: %d", indexClienteBD);

    so_debug(">");
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    int len = strlen(clientRequest.viatura.matricula);

    if (len < 5 || len > 10) {
        so_error("SD8.1", "Matrícula com tamanho inválido");
        sd11_EncerraServidorDedicado();
        exit(1);  
    }

    for (int i = 0; i < len; i++) {
        char c = clientRequest.viatura.matricula[i];
        if (!isupper(c) && !isdigit(c) && c != '-') {
            so_error("SD8.1", "Caractere inválido na matrícula: %c", c);
            sd11_EncerraServidorDedicado();
            exit(1);
        }
    }

    so_success("SD8.1", "Matrícula válida: %s", clientRequest.viatura.matricula);

    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    char *pais = clientRequest.viatura.pais;

    if (strlen(pais) != 2 || !isupper(pais[0]) || !isupper(pais[1])) {
        so_error("SD8.2", "Código de país inválido: %s", pais);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_success("SD8.2", "País válido: %s", pais);
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    char cat = clientRequest.viatura.categoria;
    if (cat != 'L' && cat != 'M' && cat != 'P') {
        so_error("SD8.3", "Categoria inválida: %c", cat);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_success("SD8.3", "Categoria válida: %c", cat);
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    FILE *fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        so_error("SD8.4", "Erro ao abrir /etc/passwd");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    char linha[512];
    int encontrado = FALSE;

    while (fgets(linha, sizeof(linha), fp)) {
        char *copy = strdup(linha);
        if (!copy) continue;

        char *token = strtok(copy, ":"); // campo 1
        for (int i = 1; i < 5; i++)      // saltar até campo 5
            token = strtok(NULL, ":");

        if (token) {
            token[strcspn(token, ",\n")] = '\0'; // tira vírgulas e \n
            if (strcmp(token, clientRequest.viatura.nomeCondutor) == 0) {
                encontrado = TRUE;
                free(copy);
                break;
            }
        }

        free(copy);
    }

    fclose(fp);

    if (encontrado) {
        so_success("SD8.4", "Nome do condutor válido: %s", clientRequest.viatura.nomeCondutor);
    } else {
        so_error("SD8.4", "Nome do condutor não encontrado em /etc/passwd");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_debug(">");
}

/**
 * @brief  sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    int segundos = so_random_between_values(1, MAX_ESPERA); 
    so_success("SD9.1", "%d", segundos);

    sleep(segundos);

    so_debug(">");
}

/**
 * @brief  sd9_2_EnviaSigusr1AoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (kill(clientRequest.pidCliente, SIGUSR1) == -1) {
        so_error("SD9.2", "Erro ao enviar SIGUSR1 ao cliente com PID=%d", clientRequest.pidCliente);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief  sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]", logFilename, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    FILE *fp = fopen(logFilename, "a+");
    if (fp == NULL) {
        so_error("SD9.3", "Erro ao abrir log file");
        sd11_EncerraServidorDedicado();
        exit(1);
    }
    if (fseek(fp, 0, SEEK_END) != 0) {  // vai para o fim do ficheiro
        so_error("SD9.3", "Erro ao posicionar no ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    *pposicaoLogfile = ftell(fp); // guarda posição
    if (*pposicaoLogfile == -1) {
        so_error("SD9.3", "Erro ao obter posição no ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    // preencher o logItem
    plogItem->viatura = clientRequest.viatura;
    time_t agora = time(NULL);
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", localtime(&agora));
    strcpy(plogItem->dataSaida, ""); // vazio porque ainda não saiu

    if (fwrite(plogItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD9.3", "Erro ao escrever log");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    fclose(fp);

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s", 
               *pposicaoLogfile, plogItem->viatura.matricula, plogItem->dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]", *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, plogItem->dataEntrada, plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 */
void sd10_1_AguardaCheckout() {
    so_debug("<");

    pause(); // Aguarda sem espera ativa

    so_success("SD10.1", "SD: A viatura %s deseja sair do parque", clientRequest.viatura.matricula);

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

    FILE *fp = fopen(logFilename, "r+b");
    if (fp == NULL) {
        so_error("SD10.2", "Erro ao abrir ficheiro de log");
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {
        so_error("SD10.2", "Erro ao posicionar no ficheiro");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    time_t agora = time(NULL);
    strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", localtime(&agora));

    // Escrever o LogItem inteiro no ficheiro binário
    if (fwrite(&logItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD10.2", "Erro ao escrever log de saída");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(1);
    }

    fclose(fp);

    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
               posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);

    sd11_EncerraServidorDedicado();
    exit(0);           
    so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    //Liberta o lugar na base de dados, envia SIGHUP ao cliente e termina o processo
    so_debug("<");

    sd11_1_LibertaLugarViatura(lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaSighupAoClienteETermina(clientRequest);

    so_debug(">");
}

/**
 * @brief  sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD >= 0) {
        // Apaga os dados da viatura
        strcpy(lugaresEstacionamento[indexClienteBD].viatura.matricula, "");
        strcpy(lugaresEstacionamento[indexClienteBD].viatura.pais, "");
        lugaresEstacionamento[indexClienteBD].viatura.categoria = '-';
        strcpy(lugaresEstacionamento[indexClienteBD].viatura.nomeCondutor, "");

        lugaresEstacionamento[indexClienteBD].pidCliente = -1;
        lugaresEstacionamento[indexClienteBD].pidServidorDedicado = -1;

        so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
    } else {
        so_debug("Lugar não estava atribuído (indexClienteBD == -1)");
    }

    so_debug(">");
}

/**
 * @brief  sd11_2_EnviaSighupAoClienteETerminaSD Ler a descrição da tarefa SD11.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (kill(clientRequest.pidCliente, SIGHUP) == -1) {
        so_error("SD11.2", "Erro ao enviar SIGHUP para Cliente");
    }
    so_success("SD11.2", "SD: Shutdown");
    so_debug(">");
    exit(0);
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd13_TrataSigusr1    Ler a descrição da tarefa SD13 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd13_TrataSigusr1(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD13", "SD: Recebi pedido do Cliente para terminar o estacionamento");
    

    so_debug(">");
}