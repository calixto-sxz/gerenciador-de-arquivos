# Protocolo e API do backup-distribuido-arquivos

Este documento é o **contrato** do sistema. Qualquer programa, em qualquer
linguagem ou sistema operacional, que siga estas regras conversa com os
servidores. O cliente C++ e o cliente Python seguem as mesmas regras.

## 1. Transporte

- TCP, porta padrão **5000** (principal) e **6001/6002** (backups).
- Mensagens em **texto**: um **cabeçalho em uma linha** (terminada em `\n`),
  campos separados por `;`, seguido opcionalmente de bytes de conteúdo.
- Em uma conexão podem ir várias requisições seguidas; os clientes do projeto
  abrem uma conexão por chamada (é mais simples).

## 2. Requisição

```
ID;COMANDO;nome;tamanho[;versao]\n
<tamanho bytes de conteúdo>
```

| Campo     | Significado |
|-----------|-------------|
| `ID`      | Identificador único da chamada (ex.: 16 dígitos hexadecimais). Não pode conter `;`. |
| `COMANDO` | `LISTAR`, `ENVIAR`, `BAIXAR`, `REMOVER` ou `REPLICAR`. |
| `nome`    | Nome do arquivo. Vazio no `LISTAR`. Sem `/`, sem `;`, sem começar com `.`, até 100 caracteres. |
| `tamanho` | Quantidade de bytes de conteúdo que vêm depois do `\n` (0 se não houver). Máximo 1 MB. |
| `versao`  | **Só no `REPLICAR`**: número da versão do arquivo. |

## 3. Resposta

```
ID;OK;tamanho\n          (seguido de <tamanho> bytes: usado no LISTAR e BAIXAR)
ID;ERRO;motivo\n         (sem conteúdo)
```

O `ID` da resposta é o mesmo da requisição.

## 4. Comandos

| Comando | Quem usa | Requisição (exemplo) | Resposta OK |
|---------|----------|----------------------|-------------|
| `LISTAR`   | cliente / backups | `a1;LISTAR;;0` | `a1;OK;11` + `nota.txt 2\n` (uma linha `nome versao` por arquivo) |
| `ENVIAR`   | cliente | `a2;ENVIAR;nota.txt;5` + `Oi!!\n` | `a2;OK;0` — o servidor cria a versão atual + 1 |
| `BAIXAR`   | cliente / backups | `a3;BAIXAR;nota.txt;0` | `a3;OK;5` + `Oi!!\n` |
| `REMOVER`  | cliente / principal→backup | `a4;REMOVER;nota.txt;0` | `a4;OK;0` |
| `REPLICAR` | **entre servidores** | `rep-a2;REPLICAR;nota.txt;5;2` + `Oi!!\n` | `rep-a2;OK;0` — só grava se a versão for **maior** que a local |

### Motivos de erro

| Motivo | Quando |
|--------|--------|
| `ARQUIVO_INEXISTENTE` | `BAIXAR`/`REMOVER` de arquivo que não existe |
| `SERVIDOR_OCUPADO`    | o semáforo de transferências ficou cheio por mais de 5 s |
| `COMANDO_INVALIDO`    | comando desconhecido |
| `FORMATO_INVALIDO`    | cabeçalho com menos de 4 campos ou números inválidos (a conexão é fechada) |
| `NOME_INVALIDO`       | nome vazio, com `/` ou `;`, ou começando com `.` |
| `ARQUIVO_GRANDE`      | `tamanho` acima de 1 MB |

## 5. Regras importantes

1. **IDs repetidos são ignorados.** Se um `ENVIAR`, `REMOVER` ou `REPLICAR`
   chegar com um ID que o servidor já atendeu, ele devolve a **mesma resposta**
   sem executar de novo. Assim, um cliente que reenviar (porque achou que a
   resposta se perdeu) não grava o arquivo duas vezes. `LISTAR` e `BAIXAR` não
   mudam nada, então não precisam disso.
2. **Versões.** Cada arquivo tem um número de versão inteiro. Todo `ENVIAR`
   soma 1. Na replicação e na sincronização P2P **vence a versão maior**.
3. **Replicação usa o ID `rep-<ID original>`**, igual em todas as tentativas:
   se o backup recebeu a primeira tentativa mas a resposta se perdeu, a segunda
   é ignorada.

## 6. Exemplo de sessão completa (o que passa no fio)

```
cliente → servidor:  7f3a;ENVIAR;nota.txt;5\nOi!!\n
servidor → cliente:  7f3a;OK;0\n
servidor → backup1:  rep-7f3a;REPLICAR;nota.txt;5;1\nOi!!\n
backup1 → servidor:  rep-7f3a;OK;0\n
cliente → servidor:  9c01;BAIXAR;outro.txt;0\n
servidor → cliente:  9c01;ERRO;ARQUIVO_INEXISTENTE\n
```

Dá até para testar "na mão" com `nc localhost 5000` e digitar `x1;LISTAR;;0`.

---

## 7. A API do sistema (o que o cliente enxerga)

A **API** é o contrato de entrada: *quais operações existem, que parâmetros
recebem e o que devolvem*. Ela não fala de sockets nem de bytes.

| Operação | Parâmetros | Devolve | Erros possíveis |
|----------|-----------|---------|-----------------|
| `listar()` | — | lista de `(nome, versão)` | — |
| `enviar(nome, conteudo)` | nome do arquivo, texto | nada (sucesso) | `NOME_INVALIDO`, `SERVIDOR_OCUPADO`, `ARQUIVO_GRANDE` |
| `baixar(nome)` | nome do arquivo | o conteúdo | `ARQUIVO_INEXISTENTE`, `SERVIDOR_OCUPADO` |
| `remover(nome)` | nome do arquivo | nada (sucesso) | `ARQUIVO_INEXISTENTE` |

Em C++ ela aparece como os métodos públicos de `GerenciadorProxy`
(`middleware/proxy.hpp`). Em Python, como as chamadas a `chamar()`.

## 8. API × Middleware

| | **API** | **Middleware** |
|---|---|---|
| O que é | O **contrato**: operações, parâmetros, resultados e erros | A **infraestrutura** que faz a chamada atravessar a rede |
| Quem enxerga | O programador do cliente | Ninguém precisa enxergar — fica escondido |
| No projeto | `listar / enviar / baixar / remover` e o formato da seção 2–4 | `rede.cpp` (sockets), `protocolo.cpp` (empacotar), `proxy.cpp` (stub), `skeleton.cpp` (despacho) |
| Pergunta que responde | "**O que** posso pedir?" | "**Como** o pedido chega lá?" |

Resumo: a API diz *o que* o sistema oferece; o middleware cuida de *como*
entregar, escondendo conexão, empacotamento, IDs, falhas e reenvio.
