// =====================================================================
// middleware/proxy.hpp — PROXY (stub) do lado do cliente
//
// O cliente chama gerenciador.enviar("nota.txt", texto) como se o objeto
// fosse local. O proxy empacota a chamada, manda pelo socket, espera a
// resposta e devolve o resultado (ou lança exceção). Isso é RPC.
// =====================================================================
#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "protocolo.hpp"

// Erro devolvido pelo servidor (ex.: ARQUIVO_INEXISTENTE, SERVIDOR_OCUPADO)
struct ErroRemoto : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class GerenciadorProxy {
public:
    // Lista de servidores "host:porta" em ordem de preferência.
    // Se o primeiro estiver fora do ar, o proxy tenta o próximo (failover).
    explicit GerenciadorProxy(std::vector<std::string> servidores);

    // ---- A "API" que o cliente enxerga ----
    std::string listar();  // linhas "nome versao"
    void enviar(const std::string& nome, const std::string& conteudo);
    std::string baixar(const std::string& nome);
    void remover(const std::string& nome);

    // Usado entre servidores (replicação e P2P)
    void replicar(const std::string& nome, const std::string& conteudo, long versao);

    // Força o ID da próxima chamada (para reenviar a MESMA requisição).
    void fixar_id(const std::string& id) { id_fixo_ = id; }
    const std::string& servidor_usado() const { return servidor_usado_; }

private:
    Resposta chamar(Requisicao req);  // empacota, envia e espera a resposta
    std::vector<std::string> servidores_;
    std::string id_fixo_;
    std::string servidor_usado_;
};
