# Roteiro de apresentação — 10 minutos, 2 pessoas

**Pessoa A** = conceitos e arquitetura · **Pessoa B** = demonstração e código.
Antes de começar: `make`, abrir 4 terminais na raiz do projeto, apagar `dados/`
e deixar o README aberto no diagrama de arquitetura.

| Tempo | Quem | O que fazer / falar |
|---|---|---|
| **0:00 – 1:00** | A | **Problema.** Um servidor de arquivos sozinho é ponto único de falha. Nossa proposta: principal + backups que se copiam, com versões e controle de concorrência. |
| **1:00 – 2:30** | A | **Arquitetura** (diagrama em camadas do README). Cliente → API → middleware → TCP → servidor. Papel de cada pasta. Mostrar o protocolo: `ID;COMANDO;nome;tamanho`. |
| **2:30 – 3:30** | A | **API × middleware.** A API é o contrato (`listar`, `enviar`, `baixar`, `remover`). O middleware é a infraestrutura escondida (sockets, empacotamento, ID, failover). Mostrar `cliente/main.cpp`: não tem nenhum socket. |
| **3:30 – 4:30** | B | **Proxy / Skeleton / RPC** (diagrama de fluxo). Abrir `middleware/proxy.cpp` (`chamar`: empacota → envia → espera) e `skeleton.cpp` (`processar`: desempacota → chama o objeto real). Citar o ID único. |
| **4:30 – 6:00** | B | **Demo ao vivo.** Terminais 1–3: `./bin/backup 6001 dados/backup1`, `./bin/backup 6002 dados/backup2`, `./bin/principal`. Terminal 4: `enviar nota.txt`, `listar`, `-s localhost:6001 listar` (replicou!). Rodar o **cliente Python** `listar` → heterogeneidade. |
| **6:00 – 7:00** | B | **Falha.** Ctrl+C no principal. `./bin/cliente baixar nota.txt` → "principal fora do ar … atendido por localhost:6001". Comentar **falha parcial** e o log de 3 tentativas. |
| **7:00 – 7:45** | A | **P2P e versões.** Backups sincronizam direto (LISTAR → compara versões → BAIXAR/REPLICAR). Por que versão e não horário: **relógios** de máquinas diferentes não batem. Citar latência (replicação assíncrona). |
| **7:45 – 9:15** | B | **Ponto principal: concorrência.** Rodar `teste/corrida.sh`. Mostrar o arquivo **misturado** com `--sem-protecao` e **íntegro** com mutex. Explicar: escrita em pedaços + pausa; mutex por arquivo; semáforo com 3 vagas → `SERVIDOR_OCUPADO`. |
| **9:15 – 10:00** | A | **Conclusão.** O que aprendemos (falha parcial, consistência, concorrência) e limitações honestas (principal não ressincroniza, remoção sem *tombstone*). Perguntas. |

## Frases-chave para não esquecer

- "O cliente **não sabe** que está usando a rede: isso é o proxy."
- "Quem garante que C++ e Python se entendem é o **protocolo**, não a linguagem."
- "Falha **parcial**: um pedaço caiu, o resto continua funcionando."
- "Versão é um **relógio lógico**: não depende da hora de cada máquina."
- "Sem mutex, o resultado depende da **ordem das threads** — isso é condição de corrida."

## Plano B (se algo falhar ao vivo)

`teste/demo_replicacao.sh` e `teste/corrida.sh` rodam a demonstração inteira
sozinhos e imprimem o resultado de cada etapa.
