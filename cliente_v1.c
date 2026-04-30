/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
 **
 ** Aluno: Inês Lage Marchante Nº:129846       Nome: Inês Marchante
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 **Este módulo funciona como um simulador do comportamento de um utilizador (cliente) que deseja usar o parque de estacionamento
 **Neste o cliente interage com o servidor através de um FIFO, envia um pedido, espera pela sua confirmação (SIGUSR1) e aguarda a ordem de saída (SIGHUP)
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"

/*** Variáveis Globais ***/
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief  Processamento do processo Cliente.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_ValidaFifoServidor(FILE_REQUESTS);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    FILE *fFifoServidor;
    c2_2_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
    c2_3_EscrevePedido(fFifoServidor, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor();
    c4_2_DesligaAlarme();
    c4_3_InputEsperaCheckout();

    c5_EncerraCliente();

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief  c1_1_ValidaFifoServidor Ler a descrição da tarefa C1.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void c1_1_ValidaFifoServidor(char *filenameFifoServidor) {
    
    //Garante que o server.fifo existe e é válido
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    struct stat info;
    if (stat(filenameFifoServidor, &info) == -1 || !S_ISFIFO(info.st_mode)) {
        so_error("C1.1", "FIFO '%s' inválido ou inexistente", filenameFifoServidor);
        exit(1);
    }

    so_success("C1.1", "FIFO do servidor '%s' está disponível", filenameFifoServidor);

    so_debug(">");
}

/**
 * @brief  c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.3 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    // Armar SIGUSR1 com sigaction (3 argumentos)
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = c6_TrataSigusr1;

    if (sigaction(SIGUSR1, &act, NULL) == -1) {
        so_error("C1.2", "Erro ao armar SIGUSR1 com sigaction");
        exit(1);
    }

    // Os restantes com signal() (1 argumento)
    if (signal(SIGHUP, c7_TrataSighup) == SIG_ERR ||
        signal(SIGINT, c8_TrataCtrlC) == SIG_ERR ||
        signal(SIGALRM, c9_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar sinais SIGHUP, SIGINT ou SIGALRM");
        exit(1);
    }

    so_success("C1.2", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief  c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param  pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(Estacionamento *pclientRequest) {
    so_debug("<");

    printf("Matrícula: ");
    so_gets(pclientRequest->viatura.matricula, 10);

    printf("País: ");
    so_gets(pclientRequest->viatura.pais, 4);

    printf("Categoria (L, M ou P): ");
    char categoriaStr[2];
    so_gets(categoriaStr, 2);
    pclientRequest->viatura.categoria = categoriaStr[0];

    printf("Nome do Condutor: ");
    so_gets(pclientRequest->viatura.nomeCondutor, 100);

    pclientRequest->pidCliente = getpid();
    pclientRequest->pidServidorDedicado = -1;

    so_success("C2.1", "%s %s %c %s %d %d",
        pclientRequest->viatura.matricula,
        pclientRequest->viatura.pais,
        pclientRequest->viatura.categoria,
        pclientRequest->viatura.nomeCondutor,
        pclientRequest->pidCliente,
        pclientRequest->pidServidorDedicado);
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->viatura.matricula, pclientRequest->viatura.pais, pclientRequest->viatura.categoria, pclientRequest->viatura.nomeCondutor, pclientRequest->pidCliente, pclientRequest->pidServidorDedicado);
}

/**
 * @brief  c2_2_AbreFifoServidor Ler a descrição da tarefa C2.2 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 */
void c2_2_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    
    //Abre o FIFO em modo de escrita 
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    *pfFifoServidor = fopen(filenameFifoServidor, "w");
    if (*pfFifoServidor == NULL) {
        so_error("C2.2", "Erro ao abrir FIFO");
        exit(1);
    }

    so_success("C2.2", "FIFO aberto para escrita");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  c2_3_EscrevePedido Ler a descrição da tarefa C2.3 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_3_EscrevePedido(FILE *fFifoServidor, Estacionamento clientRequest) {
    so_debug("< [@param fFifoServidor:%p, clientRequest:[%s:%s:%c:%s:%d:%d]]", fFifoServidor, clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);

    if (fwrite(&clientRequest, sizeof(Estacionamento), 1, fFifoServidor) != 1) {
        so_error("C2.3", "Erro ao escrever no FIFO");
        exit(1);
    }

    fflush(fFifoServidor);
    so_success("C2.3", "Pedido enviado com sucesso");
    so_debug(">");
}

/**
 * @brief  c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param  segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    //Ativa alarm(MAX_ESPERA) como tempo limite de espera por resposta
    so_debug("< [@param segundos:%d]", segundos);

    alarm(segundos);
    so_success("C3", "Espera resposta em %d segundos", segundos);
    so_debug(">");
}

/**
 * @brief  c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4 no enunciado
 */
void c4_1_EsperaRespostaServidor() {
    //Espera até receber SIGUSR1 do servidor 
    so_debug("<");

    pause();
    so_success("C4.1", "Check-in realizado com sucesso");

    so_debug(">");
}

/**
 * @brief  c4_2_DesligaAlarme Ler a descrição da tarefa C4.1 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    alarm(0);
    so_success("C4.2", "Desliguei alarme");
    so_debug(">");
}

/**
 * @brief  c4_3_InputEsperaCheckout Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_3_InputEsperaCheckout() {
    so_debug("<");

    char input[50];

    printf("Aguarde checkout...\n");

    while (TRUE) {
        printf("Se quiser fazer checkout escreva 'sair': ");
        fflush(stdout);  // força o output a aparecer

        // lê do stdin
        if (fgets(input, sizeof(input), stdin) == NULL) {
            clearerr(stdin);  // limpa o erro se fgets falhar
            continue;
        }

        // remove \n no fim se existir
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "sair") == 0) {
            so_success("C4.3", "Utilizador pretende terminar estacionamento");
            c5_EncerraCliente();
            break;
        }

        printf("Para sair, escreva exatamente: sair\n");
    }

    so_debug(">");
}

/**
 * @brief  c5_EncerraCliente      Ler a descrição da tarefa C5 no enunciado
 */
void c5_EncerraCliente() {
    so_debug("<");

    c5_1_EnviaSigusr1AoServidor(clientRequest);
    c5_2_EsperaRespostaServidorETermina();
    so_success("C5", "Cliente terminou");
    so_debug(">");
}

/**
 * @brief  c5_1_EnviaSigusr1AoServidor      Ler a descrição da tarefa C5.1 no enunciado
 * @param  clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c5_1_EnviaSigusr1AoServidor(Estacionamento clientRequest) {
    //Cliente avisa o servidor que quer sair
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]", clientRequest.viatura.matricula, clientRequest.viatura.pais, clientRequest.viatura.categoria, clientRequest.viatura.nomeCondutor, clientRequest.pidCliente, clientRequest.pidServidorDedicado);
    if (clientRequest.pidServidorDedicado <= 0) {
        so_error("C5.1", "PID do Servidor Dedicado inválido: %d", clientRequest.pidServidorDedicado);
        exit(1);
    }
    if (kill(clientRequest.pidServidorDedicado, SIGUSR1) == -1) {
        so_error("C5.1", "Erro ao enviar SIGUSR1");
        exit(1);
    }
    so_success("C5.1", "SIGUSR1 enviado ao servidor");
    so_debug(">");
}

/**
 * @brief  c5_2_EsperaRespostaServidorETermina      Ler a descrição da tarefa C5.2 no enunciado
 */
void c5_2_EsperaRespostaServidorETermina() {
    //Espera confirmação do servidor e termina 
    so_debug("<");

    pause();  // espera por SIGHUP
    so_success("C5.2", "%s", "Cliente terminou corretamente após checkout");

    exit(0);
    so_debug(">");
}

/**
 * @brief  c6_TrataSigusr1      Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataSigusr1(int sinalRecebido, siginfo_t *siginfo, void *context) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);
    Estacionamento *pRequest = &clientRequest;  

    pRequest->pidServidorDedicado = siginfo->si_pid;
    recebeuRespostaServidor = TRUE;
    so_success("C6", "Check-in concluído com sucesso pelo Servidor Dedicado %d", siginfo->si_pid);

    so_debug(">");
}

/**
 * @brief  c7_TrataSighup      Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataSighup(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C7", "Estacionamento terminado");
    printf("Estacionamento terminado\n");
    so_debug(">");
    exit(0);
}

/**
 * @brief  c8_TrataCtrlC      Ler a descrição da tarefa c8 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c8_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("C8", "Cliente: Shutdown");
    c5_EncerraCliente();
    so_debug(">");
}

/**
 * @brief  c9_TrataAlarme      Ler a descrição da tarefa c9 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c9_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C9", "Cliente: Timeout");
    exit(0);
    so_debug(">");
}