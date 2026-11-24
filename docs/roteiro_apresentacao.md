# Roteiro de estudo da apresentação

Roteiro simples, slide por slide, para estudar antes de apresentar.
**Pessoa A** = Gabriel · **Pessoa B** = Erick · tempo total: até 10 minutos.

Dica: não decorem palavra por palavra. Entendam a **ideia** de cada slide
e usem a **frase para não esquecer** como ponto de apoio.

---

## Slide 1 — Capa (Pessoa A, 20 segundos)

**O que dizer:**
Nosso trabalho é um sistema que guarda arquivos em mais de um computador.
Se um deles parar de funcionar, os arquivos continuam disponíveis nos outros.

**Frase para não esquecer:** "Guardar arquivos em vários lugares para nunca perder."

---

## Slide 2 — O problema (Pessoa A, 40 segundos)

**O que dizer:**
Quando tudo fica guardado em um computador só, qualquer problema nele afeta todo mundo.
Três coisas podem dar errado:

1. o computador desliga, e ninguém consegue pegar os arquivos;
2. o disco estraga, e os arquivos somem;
3. duas pessoas salvam o mesmo arquivo ao mesmo tempo, e ele fica bagunçado.

A nossa solução: fazer cópias automáticas em outros computadores e organizar quem salva cada arquivo.

**Frase para não esquecer:** "Um computador sozinho é um risco: se ele cai, cai tudo."

---

## Slide 3 — O sistema em quatro passos (Pessoa A, 1 minuto)

**O que dizer (seguindo o desenho):**

1. **Envia:** a pessoa manda um arquivo para o computador principal.
2. **Copia:** o principal guarda o arquivo e manda uma cópia para dois computadores reserva (os backups).
3. **Sincroniza:** os dois reservas conversam entre si e completam o que estiver faltando.
4. **Sobrevive:** se o principal parar, a pessoa pega o arquivo direto de um reserva.

Cada um desses é um programa separado, e eles só conversam pela rede. Por isso é um sistema distribuído.

**Frase para não esquecer:** "Envia, copia, sincroniza e sobrevive."

---

## Slide 4 — Cliente–servidor em camadas (Pessoa A, 1 minuto)

**O que dizer:**

- **Cliente–servidor:** um lado pede, o outro atende. Igual a um cliente e um atendente de loja.
- **Camadas:** o sistema é organizado como um prédio. Cada andar faz uma tarefa só e conversa apenas com o andar de baixo.
    - Do lado de quem pede: o programa da pessoa → o "cardápio" de pedidos → quem embrulha o pedido → a rede.
    - Do lado de quem atende: a rede → quem desembrulha o pedido → quem guarda o arquivo no disco → os servidores.
- O servidor atende várias pessoas ao mesmo tempo, porque cada uma ganha o seu próprio "atendente".

**Frase para não esquecer:** "Um pede, o outro atende; e cada andar faz uma coisa só."

**Passagem:** "Agora o Erick vai mostrar o caminho que um pedido faz."

---

## Slide 5 — O caminho de um pedido (Pessoa B, 1 minuto e 15 segundos)

**O que dizer:**
Três palavras, explicadas com um restaurante:

- **API** é o **cardápio**: mostra o que dá para pedir (listar, enviar, baixar e apagar arquivos).
- **Middleware** é o **garçom**: leva o pedido até a cozinha e traz a resposta. A pessoa não precisa saber como.
- **RPC** é pedir algo para outro computador **como se fosse no seu próprio**.

Seguindo o desenho, a pessoa pede para baixar a nota.txt:

1. o pedido sai do programa da pessoa;
2. o **proxy** embrulha o pedido numa mensagem (é o "representante" do servidor do lado de quem pede);
3. a rede leva a mensagem;
4. o **skeleton** desembrulha a mensagem do lado do servidor;
5. o servidor lê o arquivo e a resposta volta pelo mesmo caminho.

**Frase para não esquecer:** "Cardápio, garçom, e um embrulha enquanto o outro desembrulha."

---

## Slide 6 — Um protocolo comum para C++ e Python (Pessoa B, 1 minuto)

**O que dizer:**

- **Protocolo** é a regra de como escrever as mensagens, como o jeito certo de preencher um envelope.
- A nossa mensagem é uma linha com quatro partes: número do pedido, o que fazer, qual arquivo e o tamanho.
- Fizemos dois programas para a pessoa usar, um em C++ e outro em Python, escritos separadamente.
  Os dois escrevem exatamente a mesma mensagem, e o servidor atende sem saber qual linguagem foi usada.
- Isso se chama **heterogeneidade**: partes diferentes funcionando juntas.
- Testamos até um programa em Python dando ordens ao programa em C++, e os dois trocando arquivos.

**Frase para não esquecer:** "Linguagens diferentes se entendem porque seguem a mesma regra."

---

## Slide 7 — O que acontece quando algo cai (Pessoa B, 1 minuto e 15 segundos)

**O que dizer:**

- **Replicar** é guardar cópias do mesmo arquivo em vários computadores.
- **Lado esquerdo, um reserva caiu:** o principal tenta mandar a cópia 3 vezes, anota que não conseguiu e continua funcionando normalmente.
  Isso se chama **falha parcial**: uma parte caiu, o resto segue.
- **Lado direito, o principal caiu:** o programa da pessoa tenta o principal, não consegue, e vai sozinho para o reserva, que tem a cópia.
  A pessoa recebe o arquivo e nem percebe.
- Esse é o problema do slide 2 resolvido.

**Frase para não esquecer:** "Se uma parte cai, o resto continua."

**Passagem:** "O Gabriel vai mostrar como os reservas se ajudam."

---

## Slide 8 — Os backups conversam entre si (Pessoa A, 1 minuto)

**O que dizer:**

- **Peer-to-peer** quer dizer que computadores iguais conversam direto, sem ninguém no meio.
- Exemplo do nosso teste: o reserva 2 ficou desligado e perdeu um arquivo. Quando ele liga:
    1. pergunta ao reserva 1: "o que você tem?";
    2. o reserva 1 responde a lista;
    3. o reserva 2 pede só o que está faltando.
- Isso acontece quando o reserva liga e sempre que chega um arquivo novo.
- Se os dois tiverem o mesmo arquivo, fica o mais novo. Cada arquivo tem um **número de versão** que sobe a cada mudança.

**Frase para não esquecer:** "Os reservas se ajudam sozinhos, sem passar pelo principal."

**Passagem:** "Agora o Erick mostra o teste mais importante."

---

## Slide 9 — Cinco clientes gravando o mesmo arquivo (Pessoa B, 1 minuto e 30 segundos)

**O que dizer:**

- Fizemos um teste em que cinco pessoas salvam, **ao mesmo tempo**, textos diferentes no **mesmo arquivo**:
  uma só com a letra A, outra só com B, e assim por diante.
- **Sem proteção (vermelho):** todos escrevem juntos e o arquivo fica misturado, com pedaços de cada um.
- **Com proteção (azul):** o arquivo fica certinho, com o texto de uma pessoa só.
- Duas proteções:
    - **Mutex** é uma **tranca** no arquivo: enquanto um salva, os outros esperam a vez.
    - **Semáforo** é um **limite de vagas**: no máximo 3 envios ao mesmo tempo, e o quarto espera na fila.
- Esses resultados são reais, do nosso teste.

**Frase para não esquecer:** "Sem tranca, mistura. Com tranca, um de cada vez."

---

## Slide 10 — Conclusão (Pessoa A, 45 segundos)

**O que dizer:**
O sistema resolve o problema do computador único e mostra na prática todos os temas da matéria:
cliente–servidor, camadas, cardápio e garçom (API e middleware), pedido a outro computador (RPC),
linguagens diferentes juntas, reservas conversando entre si e a tranca para ninguém bagunçar o arquivo.
Obrigado, ficamos abertos a perguntas.

---

## Se a banca perguntar

| Pergunta | Resposta curta |
|---|---|
| Por que não usar o horário para saber qual arquivo é mais novo? | Cada computador pode estar com o relógio diferente. O número de versão não depende disso. |
| E se o principal voltar depois de cair? | Ele volta a atender, mas não busca o que perdeu. Seria o próximo passo do trabalho. |
| Por que o semáforo, se já existe a tranca (mutex)? | A tranca protege **um arquivo**. O semáforo limita **quantos envios ao mesmo tempo** o servidor aguenta. |
| Isso é peer-to-peer de verdade? | É misto: o principal funciona como cliente–servidor e os reservas conversam entre si como peer-to-peer. |
| Por que é um sistema distribuído se roda num computador só? | Cada parte é um programa separado e eles só conversam pela rede. Dá para colocar cada um num computador diferente sem mudar o código. |
| O que acontece se o mesmo pedido chegar duas vezes? | Cada pedido tem um número. Se o mesmo número chegar de novo, o servidor ignora e não salva duas vezes. |

---

## Antes de apresentar

- Ensaiar duas vezes com cronômetro.
- Deixar o projeto compilado (`make`) e um terminal aberto, com a fonte grande, caso queiram mostrar o teste do slide 9 (`teste/corrida.sh`).
- Levar o PDF dos slides num pendrive.
