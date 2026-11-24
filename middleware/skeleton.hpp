// =====================================================================
// middleware/skeleton.hpp — SKELETON do lado do servidor
//
// Lê a requisição do socket, desempacota, chama o objeto REAL
// (GerenciadorArquivos) e devolve a resposta empacotada.
// Também guarda os IDs já atendidos para ignorar requisições repetidas.
// =====================================================================
#pragma once
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "gerenciador_arquivos.hpp"
#include "protocolo.hpp"

class Skeleton {
public:
    explicit Skeleton(GerenciadorArquivos& objeto) : objeto_(objeto) {}

    // Chamado depois de cada alteração bem-sucedida (ENVIAR/REMOVER/REPLICAR).
    // O principal usa para replicar; o backup usa para sincronizar P2P.
    std::function<void(const Requisicao&)> apos_alteracao;

    // Fica escutando na porta; cria UMA THREAD POR CLIENTE conectado.
    void servir(int porta);

private:
    void atender_cliente(int fd);
    std::string processar(Requisicao& req);

    GerenciadorArquivos& objeto_;
    std::mutex mutex_ids_;
    std::map<std::string, std::string> respostas_por_id_;  // ID -> resposta já dada
};
