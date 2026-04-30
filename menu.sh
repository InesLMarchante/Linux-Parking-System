#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129846       Nome:Inês Lage Marchante
## Nome do Módulo: S4. Script: menu.sh
## Descrição/Explicação do Módulo:
## Este módulo permite invocar os scripts implementados no sistema do estacionamento, sem repetir eventuais funcionalidades que ja estão implementadas
##
#####################################################################################

## Este script invoca os scripts restantes, não recebendo argumentos.
## Atenção: Não é suposto que volte a fazer nenhuma das funcionalidades dos scripts anteriores. O propósito aqui é simplesmente termos uma forma centralizada de invocar os restantes scripts.
## S4.1. Apresentação:
## S4.1.1. O script apresenta (pode usar echo, cat ou outro, sem “limpar” o ecrã) um menu com as opções abaixo indicadas.

while true; do
    echo "MENU:"
    echo "1: Regista passagem - Entrada estacionamento"
    echo "2: Regista passagem - Saída estacionamento"
    echo "3: Manutenção"
    echo "4: Estatísticas"
    echo "0: Sair"
    echo ""
    echo -n "Opção: "
    read opcao

## S4.2. Validações:
## S4.2.1. Aceita como input do utilizador um número. Valida que a opção introduzida corresponde a uma opção válida. Se não for, dá so_error <opção> (com a opção errada escolhida), e volta ao passo S4.1 (ou seja, mostra novamente o menu). Caso contrário, dá so_success <opção>.

    # Validação da opção do usuário
    if [[ -z "$opcao" || ! "$opcao" =~ ^[0-4]$ ]]; then
        so_error S4.2.1 "$opcao"
        continue
    fi
    so_success S4.2.1 "$opcao"

## S4.2.2. Analisa a opção escolhida, e mediante cada uma delas, deverá invocar o sub-script correspondente descrito nos pontos S1 a S3 acima. No caso das opções 1 e 4, este script deverá pedir interactivamente ao utilizador as informações necessárias para execução do sub-script correspondente, injetando as mesmas como argumentos desse sub-script:

    if [ "$opcao" -eq 0 ]; then
    ## Apenas a opção 0 (zero) permite sair deste Script: menu.sh. Até escolher esta opção, o menu deverá ficar em ciclo, permitindo realizar múltiplas operações iterativamente (e não recursivamente, ou seja, não deverá chamar o Script: menu.sh novamente). 
        break
    fi

    if [ "$opcao" -eq 1 ]; then
    ## S4.2.2.1. Assim sendo, no caso da opção 1, o script deverá pedir ao utilizador sucessivamente e interactivamente os dados a inserir:

        echo "Regista passagem de Entrada estacionamento: "
        echo -n "Indique a matrícula da viatura: "
        read matriculaEntrada
        echo -n "Indique o código do país de origem da viatura: "
        read paisEntrada
        echo -n "Indique a categoria da viatura [L(igeiro)|P(esado)|M(otociclo)]: "
        read categoria
        echo -n "Indique o nome do condutor da viatura: "
        read nome

        if [[ -z "$matriculaEntrada" || -z "$paisEntrada" || -z "$categoria" || -z "$nome" ]]; then
            so_error S4.3 "Faltam dados na entrada."
            continue
        fi

        ./regista_passagem.sh "$matriculaEntrada" "$paisEntrada" "$categoria" "$nome"
        so_success S4.3 
        continue
    fi

    if [ "$opcao" -eq 2 ]; then
    ## Este script não deverá fazer qualquer validação dos dados inseridos, já que essa validação é feita no script S1. Após receber os dados, este script invoca o Sub-Script: regista_passagem.sh com os argumentos recolhidos do utilizador. Após a execução do sub-script, dá so_success e volta ao passo S4.1.
    ## S4.2.2.2. No caso da opção 2, o script deverá pedir ao utilizador sucessivamente e interactivamente os dados a inserir:
        echo "Regista passagem de Saída estacionamento: "
        echo -n "Indique a matrícula da viatura: "
        read matriculaSaida
        echo -n "Indique o código do país de origem da viatura: "
        read paisSaida

        if [[ -z "$matriculaSaida" || -z "$paisSaida" ]]; then
            so_error S4.4 "Faltam dados na saída."
            continue
        fi

        ./regista_passagem.sh "$paisSaida/$matriculaSaida"
        so_success S4.4  
        continue
    fi

    if [ "$opcao" -eq 3 ]; then
    ##  Este script não deverá fazer qualquer validação dos dados inseridos, já que essa validação é feita no script S1. Após receber os dados, este script invoca o Sub-Script: regista_passagem.sh com os argumentos recolhidos do utilizador. Após a execução do sub-script, dá so_success e volta ao passo S4.1.
    ## S4.2.2.3. No caso da opção 3, o script invoca o Sub-Script: manutencao.sh. Após a execução do sub-script, dá so_success e volta para o passo S4.1.
        ./manutencao.sh
        so_success S4.5
        continue
    fi

    if [ "$opcao" -eq 4 ]; then
    ## S4.2.2.4. No caso da opção 4, o script deverá pedir ao utilizador as opções de estatísticas a pedir, antes de invocar o Sub-Script: stats.sh. Se uma das opções escolhidas for a 8, o menu deverá invocar o Sub-Script: stats.sh sem argumentos, para que possa executar TODAS as estatísticas, caso contrário deve respeitar a ordem.

    echo "Estatísticas: "
    echo "1: matrículas e condutores cujas viaturas estão ainda estacionadas no parque "
    echo "2: top3 das matrículas das viaturas que passaram mais tempo estacionadas "
    echo "3: tempos de estacionamento de ligeiros e pesados agrupadas por país "
    echo "4: top3 das matrículas das viaturas que estacionaram mais tarde num dia "
    echo "5: tempo total de estacionamento por utilizador "
    echo "6: matrículas e tempo total de estacionamento delas, agrupadas por país da matrícula "
    echo "7: top3 nomes de condutores mais compridos "
    echo "8: todas as estatísticas anteriores, na ordem numérica indicada "
    echo -n "Indique quais as estatísticas a incluir, opções separadas por espaço: "
    read stats

    so_debug "$stats"
    if [[ -z "$stats" ]]; then
    so_error S4.6

    continue
    fi

    if echo -e "$stats" | grep -q "8"; then
    ./stats.sh
    so_success S4.6

    continue
    fi

    ./stats.sh $stats
    so_success S4.6

    continue
fi
done