/****************************************************************************************
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 ** Este módulo tem como função para além de iniciar a comunicação com o Servidor de recolher os dados da viatura, de enviar o pedido de estacionamento e processar todas as respostas do Servidor Dedicado,
 *  de fornecer a informação da tarifa ou encerramento.
 *  Este serve também como forma de gerir os sinais SIGINT e SIGALRM de modo a permitir o cancelamento pelo utilizador (Ctrl+C) ou timeout de espera.
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int msgId = -1;                         // Variável que tem o ID da Message Queue
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief Processamento do processo Cliente.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_GetMsgQueue(IPC_KEY, &msgId);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    c2_2_EscrevePedido(msgId, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor(msgId, &clientRequest);
    c4_2_DesligaAlarme();

    c5_MainCliente(msgId, &clientRequest);

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief c1_1_GetMsgQueue Ler a descrição da tarefa C1.1 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    *pmsgId = msgget(ipcKey, 0666);
    if (*pmsgId == -1) {
        so_error("C1.1", "Erro ao aceder à message queue");
        exit(EXIT_FAILURE);
    }
    so_success("C1.1", "");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.2 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    if (signal(SIGINT, c6_TrataCtrlC) == SIG_ERR ||
        signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao armar sinais SIGINT ou SIGALRM");
        exit(1);
    }

    so_success("C1.2", "Sinais armados com sucesso");

    so_debug(">");
}

/**
 * @brief c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(MsgContent *pclientRequest) {
    so_debug("<");

    printf("Matrícula: "); scanf("%s", pclientRequest->msgData.est.viatura.matricula);
    printf("País: "); scanf("%s", pclientRequest->msgData.est.viatura.pais);
    printf("Categoria (P/L/M): "); scanf(" %c", &pclientRequest->msgData.est.viatura.categoria);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    printf("Nome do Condutor: ");
    fgets(pclientRequest->msgData.est.viatura.nomeCondutor, sizeof(pclientRequest->msgData.est.viatura.nomeCondutor), stdin);

    pclientRequest->msgData.est.viatura.nomeCondutor[strcspn(pclientRequest->msgData.est.viatura.nomeCondutor, "\n")] = '\0';

    pclientRequest->msgData.est.pidCliente = getpid();
    pclientRequest->msgData.est.pidServidorDedicado = -1;
    pclientRequest->msgType = 1;
    pclientRequest->msgData.status = 4;

    so_success("C2.1", "%s %s %c %s %d %d",
        pclientRequest->msgData.est.viatura.matricula,
        pclientRequest->msgData.est.viatura.pais,
        pclientRequest->msgData.est.viatura.categoria,
        pclientRequest->msgData.est.viatura.nomeCondutor,
        pclientRequest->msgData.est.pidCliente,
        pclientRequest->msgData.est.pidServidorDedicado);
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c2_2_EscrevePedido Ler a descrição da tarefa C2.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_2_EscrevePedido(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

    if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), 0) == -1) {
        so_error("C2.2", "Erro ao enviar pedido");
        exit(1);
    }
    so_success("C2.2", "");
    so_debug(">");
}

/**
 * @brief c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    alarm(segundos);

    so_success("C3", "Espera resposta em %d segundos", segundos);
    so_debug(">");
}

/**
 * @brief c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    if (msgrcv(msgId, pclientRequest, sizeof(MsgContent) - sizeof(long), getpid(), 0) == -1) {
        so_error("C4.1", "Erro ao receber resposta do Servidor");
        exit(1);
    }

    recebeuRespostaServidor = TRUE;

    if (pclientRequest->msgData.status == 1) {
        so_success("C4.1", "Não é possível estacionar");
        exit(1); 
    } else if (pclientRequest->msgData.status == 0) {
        so_success("C4.1", "Check-in realizado com sucesso");
    } else {
        so_error("C4.1", "Status inválido");
        exit(1);
    }
    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief c4_2_DesligaAlarme Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    alarm(0);
    so_success("C4.2", "Desliguei alarme");
    so_debug(">");
}

/**
 * @brief c5_MainCliente Ler a descrição da tarefa C5 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c5_MainCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    while (TRUE) {
        MsgContent resposta;

        if (msgrcv(msgId, &resposta, sizeof(MsgContent) - sizeof(long), getpid(), 0) == -1) {
            so_error("C5", "Erro ao receber mensagem do Servidor Dedicado");
            exit(1);
        }
        *pclientRequest = resposta;

        if (resposta.msgData.status == INFO_TARIFA) {
            so_success("C5", "%s", resposta.msgData.infoTarifa);
        }
        else if (resposta.msgData.status == -1) {
            so_success("C5", "Não é possível estacionar");
            exit(1);
        }
        else if (resposta.msgData.status == ESTACIONAMENTO_TERMINADO) {
            so_success("C5", "Estacionamento terminado");
            exit(0);
        }
        else {
            so_error("C5", "Estado de mensagem inesperado: %d", resposta.msgData.status);
            exit(1);
        }
    }

    so_debug("> [*pclientRequest:[%s:%s:%c:%s:%d:%d]]", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.viatura.pais, pclientRequest->msgData.est.viatura.categoria, pclientRequest->msgData.est.viatura.nomeCondutor, pclientRequest->msgData.est.pidCliente, pclientRequest->msgData.est.pidServidorDedicado);
}

/**
 * @brief  c6_TrataCtrlC Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d, msgId:%d, clientRequest:[%s:%s:%c:%s:%d:%d]]", sinalRecebido, msgId, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado);

        MsgContent msg;
        msg.msgType = clientRequest.msgData.est.pidServidorDedicado;
        msg.msgData = clientRequest.msgData;
        msg.msgData.status = TERMINA_ESTACIONAMENTO;

        if (msgsnd(msgId, &msg, sizeof(msg.msgData), 0) == -1) {
            so_error("C6", "Erro ao enviar mensagem de cancelamento");
            exit(1);
        } else {
            so_success("C6", "Cliente: Shutdown");
        }
    
    so_debug(">");
}
/**
 * @brief  c7_TrataAlarme Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C7", "Cliente: Timeout");
    exit(0);

    so_debug(">");
}
