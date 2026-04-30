#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:129846       Nome:Inês Lage Marchante
## Nome do Módulo: S3. Script: stats.sh
## Descrição/Explicação do Módulo:
## Este módulo tem como objetivo criar estatísticas sobre o sistema de estacionamento Park-IUL, mostrando os resultados em formato HTML no ficheiro stats.html
## O script valida a existência e acessibilidade dos ficheiros necessários, processa dados de estacionamento e produz sete estatísticas diferentes
#####################################################################################

## Este script obtém informações sobre o sistema Park-IUL, afixando os resultados das estatísticas pedidas no formato standard HTML no Standard Output e no ficheiro stats.html. Cada invocação deste script apaga e cria de novo o ficheiro stats.html, e poderá resultar em uma ou várias estatísticas a serem produzidas, todas elas deverão ser guardadas no mesmo ficheiro stats.html, pela ordem que foram especificadas pelos argumentos do script.
## S3.1. Validações:
ficheiro="estacionamentos.txt"
ficheiro_saida="stats.html"
ano=$(date "+%Y")
mes=$(date "+%m")
ficheiro_park="arquivo-$ano-$mes.park"

if [ ! -f "$ficheiro" ]; then
    so_error "S3.1" >&2
    exit 1
fi

if [ ! -r "$ficheiro" ]; then
    so_error "S3.1" >&2
    exit 1
fi

if [ ! -f "paises.txt" ]; then
    so_error "S3.1" >&2
    exit 1
fi

if [ ! -r "paises.txt" ]; then
    so_error "S3.1" >&2
    exit 1
fi

# Verificar se existem arquivos com o formato correto
arquivos=$(find . -name "arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park")

if [ -z "$arquivos" ]; then
    so_error S3.1 "Não encontra nenhum arquivo"
    exit 1
fi

# Verificar se os arquivos encontrados têm permissão de leitura
for arquivo in $arquivos; do
    if [ ! -r "$arquivo" ]; then
        so_error S3.1 "Os arquivos não têm permissão de leitura"
        exit 1
    fi
done

if [ "$#" -gt 0 ]; then
    for arg in "$@"; do
        if ! [[ "$arg" =~ ^[1-7]$ ]]; then
            so_error "S3.1" >&2
            exit 1
        fi
    done
fi

data_atual=$(date "+%Y-%m-%d")
hora_atual=$(date "+%H:%M:%S")

## S3.2. Estatísticas:
## Cada uma das estatísticas seguintes diz respeito à extração de informação dos ficheiros do sistema Park-IUL. Caso não haja informação suficiente para preencher a estatística, poderá apresentar uma lista vazia.

cat > "$ficheiro_saida" <<EOF
<html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>
<body><h1>Lista atualizada em $data_atual $hora_atual</h1>
EOF

if [ $# -eq 0 ]; then
    args="1 2 3 4 5 6 7"
else
    args="$@"
fi

for arg in $args; do
## S3.2.1.  Obter uma lista das matrículas e dos nomes de todos os condutores cujas viaturas estão ainda estacionadas no parque, ordenados alfabeticamente por nome de condutor:
## <h2>Stats1:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## ...
## </ul>
    if [ "$arg" -eq 1 ]; then
        # Estatística 1: Matrículas e Condutores estacionados, ordenados por nome
        echo "<h2>Stats1:</h2>" >> "$ficheiro_saida"
    echo "<ul>" >> "$ficheiro_saida"
    while IFS=":" read -r matricula codigo_pais tipo condutor data_entrada data_saida; do
        
        if [ -n "$matricula" ] && [ -n "$condutor" ] && [ -z "$data_saida" ]; then
            # Imprimir a matrícula e o condutor apenas se ele está estacionado
            echo "$condutor:$matricula" 
        fi
    done < "$ficheiro" | sort -t ':' -k1 | while IFS=":" read -r condutor matricula; do
        echo "<li><b>Matrícula:</b> $matricula <b>Condutor:</b> $condutor</li>" >> "$ficheiro_saida"
    done
    echo "</ul>" >> "$ficheiro_saida"

## S3.2.2. Obter uma lista do top3 das matrículas e do tempo estacionado das viaturas que já terminaram o estacionamento e passaram mais tempo estacionadas, ordenados decrescentemente pelo tempo de estacionamento (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats2:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## </ul>
    elif [ "$arg" -eq 2 ]; then
    # Estatística 2: Top 3 matrículas com maior tempo estacionado
    echo "<h2>Stats2:</h2>" >> "$ficheiro_saida"
    echo "<ul>" >> "$ficheiro_saida"

    # Processar os dados e ordenar os top 3
    awk -F':' 'NF == 7 {soma[$1] += $7} 
        END { 
            for (matricula in soma) 
                print soma[matricula], matricula 
        }' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park | sort -nr | head -n 3 |
    while read tempo matricula; do
        echo "<li><b>Matrícula:</b> $matricula <b>Tempo estacionado:</b> $tempo</li>" >> "$ficheiro_saida"
    done

    echo "</ul>" >> "$ficheiro_saida"

## S3.2.3. Obter as somas dos tempos de estacionamento das viaturas que não são motociclos, agrupadas pelo nome do país da matrícula (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats3:</h2>
## <ul>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>

    elif [ "$arg" -eq 3 ]; then
    # Estatística 3: Somatório dos tempos por país
    echo "<h2>Stats3:</h2>" >> "$ficheiro_saida"
    echo "<ul>" >> "$ficheiro_saida"

    if [ ! -f "$ficheiro_park" ]; then
        echo "<li>Erro: Ficheiro de tempos não encontrado.</li>" >> "$ficheiro_saida"
    else
        # Calcular a soma dos tempos por país, excluindo motociclos
        awk -F':' '
        # Criar um array de países do ficheiro paises.txt
        NR==FNR {
            split($0, partes, "###")
            paises[partes[1]] = partes[2]
            next
        }

        NF == 7 && $3 != "M" {
            soma[$2] += $7
        }

        END {
            for (pais in soma) {
                # Substituir o código do país pelo nome do país, se encontrado
                print soma[pais], (paises[pais] ? paises[pais] : pais)
            }
        }' paises.txt arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park | sort -k2,2 |
        while read tempo nome_pais; do
            echo "<li><b>País:</b> $nome_pais <b>Total tempo estacionado:</b> $tempo</li>" >> "$ficheiro_saida"
        done
    fi

    echo "</ul>" >> "$ficheiro_saida"

## S3.2.4. Listar a matrícula, código de país e data de entrada dos 3 estacionamentos, já terminados ou não, que registaram uma entrada mais tarde (hora de entrada) no parque de estacionamento, ordenados crescentemente por hora de entrada:
## <h2>Stats4:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## </ul>
    elif [ "$arg" -eq 4 ]; then
    
    # Estatística 4: Estacionamentos com entrada mais recente (ordenado por hora de entrada, crescente)
    echo "<h2>Stats4:</h2>" >> "$ficheiro_saida"
    echo "<ul>" >> "$ficheiro_saida"

    # Extrai a hora de entrada corretamente e ordena
    top3DatasEntradaOrdenada=$(awk -F':' '{gsub(/h/, "", $5); print $5}' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park estacionamentos.txt 2>/dev/null | 
        awk -F'T' '{print $2}' | sort -g | tail -n 3 | sed 's/\([0-9][0-9]\)\([0-9][0-9]\)/\1h\2/')

    # Para cada hora encontrada, busca os dados completos correspondentes
    for data in $top3DatasEntradaOrdenada; do
        matriculaPaisEDataEntrada=$(awk -F':' -v data="$data" '$5 ~ data {print "<li><b>Matrícula:</b> " $1 " <b>País:</b> " $2 " <b>Data Entrada:</b> " $5 "</li>"; exit}' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park estacionamentos.txt 2>/dev/null)
        
        echo "$matriculaPaisEDataEntrada" >> "$ficheiro_saida"
    done

    echo "</ul>" >> "$ficheiro_saida"

## S3.2.5. Tendo em consideração que um utilizador poderá ter várias viaturas, determine o tempo total, medido em dias, horas e minutos gasto por cada utilizador da plataforma (ou seja, agrupe os minutos em dias e horas).
## <h2>Stats5:</h2>
## <ul>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## ...
## </ul>
    elif [ "$arg" -eq 5 ]; then
    echo "<h2>Stats5:</h2>" >> stats.html
    echo "<ul>" >> stats.html

    # Verifica se existem arquivos que correspondem ao padrão
    arquivos=$(ls arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park 2>/dev/null)

    if [ -z "$arquivos" ]; then
        echo "<li><b>Erro:</b> Nenhum arquivo de dados encontrado.</li>" >> stats.html
        echo "</ul>" >> stats.html
        exit 1
    fi

    nomes_por_ordem=$(awk -F':' '{print $4}' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park | sort -u | sed 's/ //')

    for nome in $nomes_por_ordem; do

        nome=$(echo "$nome" | sed 's/\([a-z]\)\([A-Z]\)/\1 \2/g')

        # Calcula o tempo total para cada condutor
        nome_com_tempo=$(awk -F':' -v nome="$nome" '$4 ~ nome {soma += $7} 
        END {
            dias = int(soma / 1440);
            horas = int((soma % 1440) / 60);
            minutos = soma % 60;
            if (soma > 0) {
                print "<li><b>Condutor:</b> " nome " <b>Tempo total:</b> " dias " dia(s), " horas " hora(s) e " minutos " minuto(s)</li>";
            }
        }' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park)

        # Adiciona o tempo total ao arquivo HTML
        if [ -n "$nome_com_tempo" ]; then
            echo "$nome_com_tempo" >> stats.html
        fi
    done

    echo "</ul>" >> stats.html
    echo "" >> stats.html

## S3.2.6. Liste as matrículas das viaturas distintas e o tempo total de estacionamento de cada uma, agrupadas pelo nome do país com um totalizador de tempo de estacionamento por grupo, e totalizador de tempo global.
## <h2>Stats6:</h2>
## <ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## ...
## </ul>

    elif [ "$arg" -eq 6 ]; then
    echo "<h2>Stats6:</h2>" >> stats.html
    echo "<ul>" >> stats.html

    paises=$(awk -F':' '{print $2}' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park | sort -u)

    for codigo_pais in $paises; do

        tempo_total_pais=$(awk -F':' -v pais="$codigo_pais" '$2 == pais {total += $7} END {print total}' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park)

        # Traduzir o código do país para o nome completo
        nome_do_pais=$(grep "^$codigo_pais###" paises.txt | awk -F'###' '{print $2}')

        echo "<li><b>País:</b> $nome_do_pais <b>Total tempo estacionado:</b> $tempo_total_pais</li>" >> stats.html
        echo "<ul>" >> stats.html

        matriculas_estacionamento=$(awk -F':' -v pais="$codigo_pais" '
        $2 == pais { 
            matriculas[$1] += $7 
        } 
        END { 
            for (matricula in matriculas) { 
                print "<li><b>Matrícula:</b> " matricula " <b> Total tempo estacionado:</b> " matriculas[matricula] "</li>"
            }
        }' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park)

        # Adicionar as estatísticas de matrícula ao arquivo
        echo "$matriculas_estacionamento" >> stats.html
        echo "</ul>" >> stats.html
    done
    echo "</ul>" >> $ficheiro_saida
    echo "" >> $ficheiro_saida
    
## S3.2.7. Obter uma lista do top3 dos nomes mais compridos de condutores cujas viaturas já estiveram estacionadas no parque (ou que ainda estão estacionadas no parque), ordenados decrescentemente pelo tamanho do nome do condutor:
## <h2>Stats7:</h2>
## <ul>
## <li><b> Condutor:</b> <Nome do Condutor mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor segundo mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor terceiro mais comprido></li>
## </ul>
    elif [ "$arg" -eq 7 ]; then
    # Estatística 7: Top 3 nomes mais compridos de condutores
    echo "<h2>Stats7:</h2>" >> "$ficheiro_saida"
    echo "<ul>" >> "$ficheiro_saida"

    # Usar awk para processar o arquivo "arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park"
    awk -F':' '{
        # Extrai o nome completo do condutor (campo 4)
        nome = $4
        gsub(/^ +| +$/, "", nome)  # Remove espaços extras à esquerda/direita
        gsub(/<.*>/, "", nome)      # Remove tags HTML

        # Se o nome não for vazio, calcula o comprimento e imprime
        if (length(nome) > 0) {
            print length(nome), nome
        }
    }' arquivo-[0-9][0-9][0-9][0-9]-[0-9][0-9].park | \
    # Ordena os nomes pelo comprimento (ordem decrescente)
    sort -nr | \

    head -n 3 | \
    # Formata os resultados para HTML e garante que o nome está limpo
    awk 'BEGIN {FS=" "}{print "<li><b> Condutor:</b> " substr($0, index($0,$2)) "</li>"}' | \

    sed '/^$/d' >> "$ficheiro_saida"

    echo "</ul>" >> "$ficheiro_saida"
    echo "" >> "$ficheiro_saida"
fi
done

## S3.3. Processamento do script:
## S3.3.1. O script cria uma página em formato HTML, chamada stats.html, onde lista as várias estatísticas pedidas.
## O ficheiro stats.html tem o seguinte formato:
## <html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>
## <body><h1>Lista atualizada em <Data Atual, formato AAAA-MM-DD> <Hora Atual, formato HH:MM:SS></h1>
## [html da estatística pedida]
## [html da estatística pedida]
## ...
## </body></html>
## Sempre que o script for chamado, deverá:
## • Criar o ficheiro stats.html.
## • Preencher, neste ficheiro, o cabeçalho, com as duas linhas HTML descritas acima, substituindo os campos pelos valores de data e hora pelos do sistema.
## • Ciclicamente, preencher cada uma das estatísticas pedidas, pela ordem pedida, com o HTML correspondente ao indicado na secção S3.2.
## • No final de todas as estatísticas preenchidas, terminar o ficheiro com a última linha “</body></html>”

echo "</body></html>" >> "$ficheiro_saida"

so_success "S3.1"
so_success "S3.3"