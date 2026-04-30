#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº: 129846      Nome:Inês Lage Marchante
## Nome do Módulo: S1. Script: regista_passagem.sh
## Descrição/Explicação do Módulo:
## Este módulo tem como objetivo principal gerir as entradas e saídas de viaturas no parque de estacionamento Park-IUL, validando
## e registando os dados de cada veículo e condutor
#####################################################################################

## Este script é invocado quando uma viatura entra/sai do estacionamento Park-IUL. Este script recebe todos os dados por argumento, na chamada da linha de comandos, incluindo os <Matrícula:string>, <Código País:string>, <Categoria:char> e <Nome do Condutor:string>.

## S1.1. Valida os argumentos passados e os seus formatos:

if [ $# -ne 1 ] && [ $# -ne 4 ]; then
    so_error S1.1 "Número de argumentos inválido"
    exit 1
fi

if [ $# -eq 4 ]; then
    matricula=$1
    codigo_pais=$2
    categoria=$3
    nome=$4

    if [ ! -f paises.txt ] || ! grep -q "^$codigo_pais###" paises.txt; then
        so_error S1.1 "Codigo de país inválido ou não encontrado em paises.txt"
        exit 1
    fi

    # Verifica se o nome tem pelo menos 2 palavras
    if [ "$(echo "$nome" | awk '{print NF}')" -lt 2 ] || ! cut -d: -f5 /etc/passwd | awk -F ',' '{print $1}' | awk '{print $1, $NF}' | grep -Fxq "$nome"; then
        so_error "S1.1"
        exit 1
    fi

    # Verificação do formato da matrícula
    regex=$(grep "^$codigo_pais###" paises.txt | awk -F'###' '{print $3}' | tr -d '\r')
    if [ -z "$regex" ] || [[ ! $matricula =~ $regex ]]; then
        so_error "S1.1"
        exit 1
    fi
    so_success "S1.1"

    # Se a categoria for passada, verifica se é válida
    if [[ "$categoria" != "L" && "$categoria" != "P" && "$categoria" != "M" ]]; then
        so_error "S1.1"
        exit 1
    fi
fi

if [ $# -eq 1 ]; then
    # Extrai o código do país e matrícula do argumento
    codigo_pais=$(echo "$1" | cut -d'/' -f1)
    matricula=$(echo "$1" | cut -d'/' -f2)

    if [ ! -f paises.txt ] || ! grep -q "^$codigo_pais###" paises.txt; then
        so_error S1.1 "Código de país inválido ou não encontrado em paises.txt"
        exit 1
    fi

    regex=$(grep "^$codigo_pais###" paises.txt | awk -F'###' '{print $3}' | tr -d '\r')
    if [ -z "$regex" ] || [[ ! $matricula =~ $regex ]]; then
        so_error S1.1 "Formato de matrícula inválido"
        exit 1
    fi
    so_success "S1.1"
fi

## • Valida se os argumentos passados são em número suficiente (para os dois casos exemplificados), assim como se a formatação de cada argumento corresponde à especificação indicada. O argumento <Categoria> pode ter valores: L (correspondente a Ligeiros), P (correspondente a Pesados) ou M (correspondente a Motociclos);
## • A partir da indicação do argumento <Código País>, valida se o argumento <Matrícula> passada cumpre a especificação da correspondente <Regra Validação Matrícula>;
## • Valida se o argumento <Nome do Condutor> é o “primeiro + último” nomes de um utilizador atual do Tigre;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.1.

## S1.2. Valida os dados passados por argumento para o script com o estado da base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:

ficheiro="estacionamentos.txt"

if [ $# -eq 4 ]; then

    matricula_limpa=$(echo "$1" | tr -d ' -"' | tr -d '-')  # Remove espaços, hifens e aspas
    data_atual=$(date '+%FT%Hh%M')

    if [ ! -f "$ficheiro" ]; then
        touch "$ficheiro" 
    fi

    # Verifica se a matrícula já está registrada
    if grep -q -E "^$matricula_limpa:" "$ficheiro"; then
        numero_colunas=$(grep -E "^$matricula_limpa:" "$ficheiro" | awk -F: '{print NF}')
        
        # Se já existe um registro com 5 colunas (entrada sem saída), dá erro
        if echo "$numero_colunas" | grep -q "5"; then
            so_error "S1.2"
            exit 1
        fi
    fi
    # Registrar a entrada da viatura
    echo "$matricula_limpa:$2:$3:$4:$data_atual" >> "$ficheiro"
    so_success "S1.2"
    
fi

if [ $# -eq 1 ]; then
    matricula_limpa=$(echo "$1" | cut -d'/' -f2 | tr -d ' -"' | tr -d '-')  # Remove espaços, hifens e aspas

    if [ ! -f "$ficheiro" ]; then
        touch "$ficheiro"  
    fi

    # Verifica se a matrícula está no ficheiro
    if ! grep -q -E "^$matricula_limpa:" "$ficheiro"; then
        so_error "S1.2"
        exit 1
    fi

    # Verifica quantas colunas tem o registo da matrícula
    numero_colunas=$(grep -E "^$matricula_limpa:" "$ficheiro" | awk -F: '{print NF}')

    if ! echo -e "$numero_colunas" | grep -q "5"; then
       so_error S1.2 "Registo da matrícula nao tem 5 colunas" 
       exit 1
    fi
    so_success "S1.2"
fi

## • Valida se, no caso de a invocação do script corresponder a uma entrada no parque de estacionamento, se ainda não existe nenhum registo desta viatura na base de dados;
## • Valida se, no caso de a invocação do script corresponder a uma saída do parque de estacionamento, se existe um registo desta viatura na base de dados;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.2.


## S1.3. Atualiza a base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:

ficheiro="estacionamentos.txt"
data_atual=$(date +"%Y-%m-%dT%Hh%M")

# Limpar a matrícula de caracteres especiais
matricula_limpa=$(echo "$1" | tr -cd 'A-Za-z0-9/')

# Verificar se o ficheiro existe e tem permissões de escrita
if [ ! -w "$ficheiro" ]; then
    so_error "S1.3"
    exit 1
fi
so_success S1.3

if [ ! -f "$ficheiro" ]; then 
    so_error S1.3 "O ficheiro nao foi encontrado"
    exit 1
fi

# Se receber 4 argumentos, registar uma nova entrada
if [ "$#" -eq 4 ]; then
    
    if ! grep -q "^${matricula_limpa}:" "$ficheiro"; then
    # Matrícula não encontrada, podemos registrar a entrada
    echo "$matricula_limpa:$2:$3:$4:$data_atual" >> "$ficheiro"
    so_success "S1.3"
    fi
fi

# Se receber 1 argumento, registar a saída
if [ "$#" -eq 1 ]; then
    codigo_pais=$(echo "$1" | cut -d'/' -f1)
    matricula=$(echo "$1" | cut -d'/' -f2)
    matricula_saida=$(echo "$matricula" | tr -cd 'A-Za-z0-9')

    # Procurar a linha que contém a matrícula e tem apenas 5 colunas
    linha_encontrada=$(grep -E "^${matricula}|^${matricula_saida}" "$ficheiro" | awk -F: 'NF == 5 {print $0}')

    if [ -n "$linha_encontrada" ]; then
        # Adicionar a data de saída
        nova_linha="${linha_encontrada}:${data_atual}"
        sed -i "s|^${linha_encontrada}$|${nova_linha}|" "$ficheiro"
    fi
fi

## • Remova do argumento <Matrícula> passado todos os separadores (todos os caracteres que não sejam letras ou números) eventualmente especificados;
## • Especifique como data registada (de entrada ou de saída, conforme o caso) a data e hora do sistema Tigre;
## • No caso de um registo de entrada, crie um novo registo desta viatura na base de dados;
## • No caso de um registo de saída, atualize o registo desta viatura na base de dados, registando a data de saída;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.3 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.3.

## S1.4. Lista todos os estacionamentos registados, mas ordenados por saldo:

## • O script deve criar um ficheiro chamado estacionamentos-ordenados-hora.txt igual ao que está no ficheiro estacionamentos.txt, com a mesma formatação, mas com os registos ordenados por ordem crescente da hora (e não da data) de entrada das viaturas.
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.4 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.4.

ficheiro_ordenado="estacionamentos-ordenados-hora.txt"
ficheiro="estacionamentos.txt"

if [ ! -f "$ficheiro" ]; then
    so_error S1.4 "O ficheiro nao foi encontrado"
    exit 1
fi

# Criar uma cópia do ficheiro estacionamentos.txt
cp "$ficheiro" "$ficheiro_ordenado"

sed -i "s/\([0-9]\)[T]\([0-9]\)/\1:\2/" "$ficheiro_ordenado"
sed -i "s/\([0-9]\)[h]\([0-9]\)/\1\2/" "$ficheiro_ordenado"

# Ordenar por hora (coluna 6) de forma numérica
sort -t: -k6,6n -o "$ficheiro_ordenado" "$ficheiro_ordenado"

awk -F: '{ $6 = substr($6, 1, 2) "h" substr($6, 3, 2); print $1":"$2":"$3":"$4":"$5"T"$6":"$7 }' "$ficheiro_ordenado" > temporario.txt && mv temporario.txt "$ficheiro_ordenado"
sed -i 's/:$//' "$ficheiro_ordenado"
so_success "S1.4"