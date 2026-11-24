#!/usr/bin/env bash
# =====================================================================
# teste/corrida.sh — CONDIÇÃO DE CORRIDA (ponto principal da demonstração)
#
# Vários clientes enviam, AO MESMO TEMPO, conteúdos diferentes para o
# MESMO arquivo. Roda duas vezes:
#   1) servidor com --sem-protecao  -> esperado: arquivo MISTURADO
#   2) servidor normal (mutex+sem.) -> esperado: arquivo ÍNTEGRO
# Uso: teste/corrida.sh [numero_de_clientes]   (padrão: 5)
# =====================================================================
cd "$(dirname "$0")/.." || exit 1
N=${1:-5}
PORTA=5099                          # porta separada, não atrapalha a demo
PASTA=dados/teste_corrida
TMP=$(mktemp -d)
make -s principal cliente || exit 1

# Cada cliente i manda 30 linhas só com a sua letra (AAAA..., BBBB..., ...)
LETRAS=(A B C D E F G H I J)
for ((i = 0; i < N; i++)); do
    L=${LETRAS[$((i % 10))]}
    for ((j = 0; j < 30; j++)); do printf "cliente-%s %s\n" "$L" "$(printf "$L%.0s" {1..19})"; done > "$TMP/c$i.txt"
done

rodar() {
    local flag=$1
    echo
    echo "=================================================================="
    echo " Servidor: ./bin/principal $PORTA $PASTA /dev/null $flag"
    echo "=================================================================="
    rm -rf "$PASTA"
    # /dev/null como lista de backups: o teste isola o servidor principal
    ./bin/principal $PORTA $PASTA /dev/null $flag > "$TMP/servidor.log" 2>&1 &
    local pid=$!
    sleep 0.5

    for ((i = 0; i < N; i++)); do   # dispara todos os clientes juntos
        ./bin/cliente -s localhost:$PORTA enviar "$TMP/c$i.txt" compartilhado.txt > /dev/null 2>&1 &
    done
    wait $(jobs -p | grep -v "^$pid$") 2>/dev/null

    ./bin/cliente -s localhost:$PORTA baixar compartilhado.txt "$TMP/final.txt" > /dev/null 2>&1
    kill $pid 2>/dev/null; wait $pid 2>/dev/null

    echo "Primeiras 8 linhas do arquivo final:"
    head -n 8 "$TMP/final.txt" | cat -v | sed 's/^/   | /'
    echo "   ($(wc -c < "$TMP/final.txt") bytes; cada cliente enviou $(wc -c < "$TMP/c0.txt") bytes)"
    for ((i = 0; i < N; i++)); do
        if cmp -s "$TMP/final.txt" "$TMP/c$i.txt"; then
            echo ">>> RESULTADO: ÍNTEGRO — igual ao conteúdo do cliente ${LETRAS[$i]} (só um venceu)"
            return
        fi
    done
    echo ">>> RESULTADO: CORROMPIDO — mistura de $(grep -o 'cliente-.' "$TMP/final.txt" | sort -u | wc -l) clientes diferentes"
}

echo "Teste de corrida com $N clientes enviando para o MESMO arquivo"
rodar --sem-protecao
rodar ""
rm -rf "$TMP" "$PASTA"
