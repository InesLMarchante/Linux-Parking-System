#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129846       Nome:Inês Lage Marchante
## Nome do Módulo: S2. Script: manutencao.sh
## Descrição/Explicação do Módulo:
## Este módulo tem como objetivo a execução dos registos de estacionamento, garantindo a validade dos dados e arquivando os registos já concluídos, ou seja com data de entrada e data de saída
##
#####################################################################################

## Este script não recebe nenhum argumento, e permite realizar a manutenção dos registos de estacionamento. 

## S2.1. Validações do script:

if [ ! -f paises.txt ]; then
    so_error S2.1 "O ficheiro paises.txt nao existe"
    exit 1
fi

if [ ! -f estacionamentos.txt ]; then
    so_success "S2.1"
    so_success "S2.2"
    exit 0
fi

if [ ! -s estacionamentos.txt ]; then
    so_success S2.1 "O ficheiro nao foi encontrado"
    so_success S2.2 "Nao há registos no ficheiro"
    exit 0
fi

while IFS=":" read -r matricula codigo_pais categoria nome data_entrada data_saida; do
    # Verificar se a matrícula está vazia
    if [ -z "$matricula" ]; then
        continue
    fi

    # Validar o código do país
    if ! grep -q "^$codigo_pais###" paises.txt; then 
        so_error "S2.1"
        exit 1
    fi

    # Obter a regex para validar a matrícula
    regex=$(grep "^$codigo_pais###" paises.txt | awk -F'###' '{print $3}' | tr -d '\r')

    if [ -z "$regex" ]; then
        so_error S2.1 "Erro ao validar a matrícula para o país $codigo_pais."
        exit 1
    fi

    if ! echo "$matricula" | grep -E "$regex" > /dev/null; then
        so_error S2.1 "A matricula nao corresponde ao formato esperado"
        exit 1
    fi 

    # Verificar se as datas estão válidas
    data_entrada=$(echo "$data_entrada" | sed 's/T/ /; s/h/:/')
    timestamp_entrada=$(date -d "$data_entrada" "+%s" 2>/dev/null)
    if [ $? -ne 0 ] || [ -z "$timestamp_entrada" ]; then
        so_error S2.1 "Data de entrada inválida: $data_entrada"
        exit 1
    fi

     # Só verificar a data de saída se ela existir
    if [ -n "$data_saida" ]; then
        data_saida=$(echo "$data_saida" | sed 's/T/ /; s/h/:/')
        timestamp_saida=$(date -d "$data_saida" "+%s" 2>/dev/null)
        
        if [ $? -ne 0 ] || [ -z "$timestamp_saida" ]; then
            so_error S2.1 "Data de saida inválida"
            exit 1
        fi

        # Verificar se a data de saída é maior que a de entrada
        if [ "$timestamp_saida" -le "$timestamp_entrada" ]; then
            so_error S2.1 "Data de saída menor que a de entrada"
            exit 1
        fi
    fi 
done < estacionamentos.txt    
    so_success "S2.1"

## O script valida se, no ficheiro estacionamentos.txt:
## • Todos os registos referem códigos de países existentes no ficheiro paises.txt;
## • Todas as matrículas registadas correspondem à especificação de formato dos países correspondentes;
## • Todos os registos têm uma data de saída superior à data de entrada;
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.1.


## S2.2. Processamento:
# Verificar se o arquivo estacionamentos.txt existe
# Verificar se o arquivo estacionamentos.txt existe e não está vazio
# Verificar se o arquivo estacionamentos.txt existe e não está vazio
# Verificar se o arquivo estacionamentos.txt existe e não está vazio

if [ ! -f "estacionamentos.txt" ]; then
    so_success S2.2 "O ficheiro nao existe"
    exit 0 
fi

if [ ! -s "estacionamentos.txt" ]; then
    so_error S2.2 "O ficheiro está vazio"
    exit 1
fi

while IFS=":" read -r matricula codigo_pais categoria nome data_entrada data_saida; do
    # Verificar se há data de saída
    if [ -n "$data_saida" ]; then
        # Converter o formato da data de entrada e saída para o formato adequado 
        data_entrada_formatada=$(echo "$data_entrada" | sed 's/T/ /; s/h/:/')
        data_saida_formatada=$(echo "$data_saida" | sed 's/T/ /; s/h/:/')

        # Converter as datas para timestamps
        timestamp_entrada=$(date -d "$data_entrada_formatada" +%s)
        timestamp_saida=$(date -d "$data_saida_formatada" +%s)

        # Verificar se as conversões foram realizadas corretamente
        if [ -z "$timestamp_entrada" ] || [ -z "$timestamp_saida" ]; then
            so_error S2.2 "Erro ao converter as datas"
            exit 1
        fi

        # Calcular a diferença em minutos
        tempo_estacionado=$(( (timestamp_saida - timestamp_entrada) / 60 ))

        # Extrair ano e mês da data de saída
        ano=$(echo "$data_saida" | cut -d '-' -f1)
        mes=$(echo "$data_saida" | cut -d '-' -f2)

        # Definir o arquivo de destino baseado no ano e mês
        ficheiro_destino="arquivo-${ano}-${mes}.park"

        echo "$matricula:$codigo_pais:$categoria:$nome:$data_entrada:$data_saida:$tempo_estacionado" >> "$ficheiro_destino"
        if [ $? -ne 0 ]; then
            so_error S2.2 "Erro ao escrever no arquivo"
            exit 1
        fi

        # Remover o registo do arquivo estacionamentos.txt
        sed -i "/^$matricula:$codigo_pais:$categoria:$nome:$data_entrada/d" estacionamentos.txt
        if [ $? -ne 0 ]; then
            so_error "S2.2"
            exit 1
        fi
    fi
done < estacionamentos.txt

so_success "S2.2"

## • O script move, do ficheiro estacionamentos.txt, todos os registos que estejam completos (com registo de entrada e registo de saída), mantendo o formato do ficheiro original, para ficheiros separados com o nome arquivo-<Ano>-<Mês>.park, com todos os registos agrupados pelo ano e mês indicados pelo nome do ficheiro. Ou seja, os registos são removidos do ficheiro estacionamentos.txt e acrescentados ao correspondente ficheiro arquivo-<Ano>-<Mês>.park, sendo que o ano e mês em questão são os do campo <DataSaída>. 
## • Quando acrescentar o registo ao ficheiro arquivo-<Ano>-<Mês>.park, este script acrescenta um campo <TempoParkMinutos> no final do registo, que corresponde ao tempo, em minutos, que durou esse registo de estacionamento (correspondente à diferença em minutos entre os dois campos anteriores).
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.2.
## • O registo em cada ficheiro arquivo-<Ano>-<Mês>.park, tem então o formato:
## <Matrícula:string>:<Código País:string>:<Categoria:char>:<Nome do Condutor:string>: <DataEntrada:AAAA-MM-DDTHHhmm>:<DataSaída:AAAA-MM-DDTHHhmm>:<TempoParkMinutos:int>
## • Exemplo de um ficheiro arquivo-<Ano>-<Mês>.park, para janeiro de 2025:

