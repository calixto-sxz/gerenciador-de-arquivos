// =====================================================================
// middleware/protocolo.hpp — EMPACOTAMENTO das mensagens (o "contrato")
//
// Requisição: ID;COMANDO;nome;tamanho[;versao]\n  + <tamanho> bytes
// Resposta:   ID;OK;tamanho\n + <tamanho> bytes    ou   ID;ERRO;motivo\n
// Veja docs/protocolo.md.
// =====================================================================
#pragma once
#include <string>

const long TAMANHO_MAXIMO = 1024 * 1024;  // arquivos pequenos: até 1 MB

struct Requisicao {
    std::string id;        // identificador único da chamada
    std::string comando;   // LISTAR, ENVIAR, BAIXAR, REMOVER, REPLICAR
    std::string nome;      // nome do arquivo (vazio no LISTAR)
    long versao = 0;       // só no REPLICAR (5º campo)
    std::string conteudo;  // bytes que vêm depois do cabeçalho
};

struct Resposta {
    std::string id;
    bool ok = false;
    std::string motivo;    // preenchido quando ERRO
    std::string conteudo;  // preenchido quando OK (LISTAR, BAIXAR)
};

std::string montar_requisicao(const Requisicao& req);
// Devolve "" se leu certo, ou um código: CONEXAO_FECHADA, FORMATO_INVALIDO...
std::string ler_requisicao(int fd, Requisicao& req);

std::string resposta_ok(const std::string& id, const std::string& conteudo);
std::string resposta_erro(const std::string& id, const std::string& motivo);
bool ler_resposta(int fd, Resposta& resp);
