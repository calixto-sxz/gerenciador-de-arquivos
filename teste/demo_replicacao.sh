#!/usr/bin/env bash
# =====================================================================
# teste/demo_replicacao.sh — demonstração automática (os 4 terminais do
# README num script só). Logs dos servidores ficam em dados/logs/.
#  1. envio com replicação        4. P2P: backup que estava fora se atualiza
#  2. principal cai -> backup     5. falha parcial: 3 tentativas + log
#  3. cliente Python no backup    6. ID repetido é ignorado
# ATENÇÃO: apaga dados/principal, dados/backup1 e dados/backup2.
# =====================================================================
cd "$(dirname "$0")/.." || exit 1
make -s || exit 1
rm -rf dados/principal dados/backup1 dados/backup2 dados/logs dados/falhas_replicacao.log
mkdir -p dados/logs

titulo() { echo; echo "---------- $* ----------"; }
cli() { echo "\$ cliente $*"; ./bin/cliente "$@" 2>&1 | sed 's/^/  /'; }
liga_backup() { ./bin/backup "$1" "dados/backup$2" >> "dados/logs/backup$2.log" 2>&1 & eval "B$2=$!"; }
liga_principal() { ./bin/principal >> dados/logs/principal.log 2>&1 & P=$!; }

liga_backup 6001 1; liga_backup 6002 2; liga_principal
sleep 0.5

titulo "1. Envio com replicação"
echo "Olá, Sistemas Distribuídos! (versão 1)" > dados/nota.txt
cli enviar dados/nota.txt
sleep 1
echo "Principal:"; cli -s localhost:5000 listar
echo "Backup 1:";  cli -s localhost:6001 listar
echo "Backup 2:";  cli -s localhost:6002 listar

titulo "2. Derrubando o PRINCIPAL e baixando do backup (failover no proxy)"
kill $P; wait $P 2>/dev/null
cli baixar nota.txt

titulo "3. Cliente PYTHON baixando direto do backup 2"
echo "\$ python3 cliente_python/cliente.py -s localhost:6002 baixar nota.txt"
python3 cliente_python/cliente.py -s localhost:6002 baixar nota.txt 2>&1 | sed 's/^/  /'

titulo "4. Peer-to-peer: backup 2 desligado, arquivo novo vai só pro backup 1"
kill $B2; wait $B2 2>/dev/null
echo "Arquivo criado enquanto o principal e o backup 2 estavam fora" > dados/novo.txt
cli enviar dados/novo.txt
echo "Religando o backup 2 (ele sincroniza com o vizinho ao ligar)..."
liga_backup 6002 2; sleep 1
echo "Backup 2:"; cli -s localhost:6002 listar
grep "\[p2p\]" dados/logs/backup2.log | tail -n 2 | sed 's/^/  log backup2: /'

titulo "5. Falha parcial: principal volta, backup 2 cai, principal tenta 3x"
liga_principal; kill $B2; wait $B2 2>/dev/null; sleep 0.5
echo "versão 2" > dados/nota.txt
cli enviar dados/nota.txt
sleep 3.5
grep "\[replicacao\]" dados/logs/principal.log | tail -n 5 | sed 's/^/  log principal: /'
echo "dados/falhas_replicacao.log:"; sed 's/^/  /' dados/falhas_replicacao.log

titulo "6. Mesmo ID enviado duas vezes (o servidor ignora o repetido)"
cli -s localhost:5000 duplicar dados/nota.txt
cli -s localhost:5000 listar
grep "repetido" dados/logs/principal.log | sed 's/^/  log principal: /'

kill $P $B1 2>/dev/null; wait 2>/dev/null
echo; echo "Fim. Logs completos em dados/logs/"
