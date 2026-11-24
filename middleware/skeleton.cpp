// =====================================================================
// middleware/skeleton.cpp — despacha a chamada remota para o objeto real
// =====================================================================
#include "skeleton.hpp"

#include <cstdlib>
#include <stdexcept>
#include <thread>

#include "rede.hpp"

void Skeleton::servir(int porta) {
    int servidor = escutar(porta);
    if (servidor < 0) {
        registrar("ERRO: não consegui escutar na porta " + std::to_string(porta) +
                  " (já está em uso?)");
        std::exit(1);
    }
    registrar("Escutando na porta " + std::to_string(porta));
    while (true) {
        int cliente = aceitar(servidor);
        if (cliente < 0) continue;
        // Uma thread por cliente: vários clientes são atendidos ao mesmo tempo.
        std::thread(&Skeleton::atender_cliente, this, cliente).detach();
    }
}

void Skeleton::atender_cliente(int fd) {
    while (true) {
        Requisicao req;
        std::string erro = ler_requisicao(fd, req);  // desempacota
        if (erro == "CONEXAO_FECHADA") break;
        if (!erro.empty()) {  // cabeçalho com defeito: avisa e encerra
            registrar("[skeleton] requisição inválida: " + erro);
            enviar_tudo(fd, resposta_erro(req.id.empty() ? "?" : req.id, erro));
            break;
        }
        if (!enviar_tudo(fd, processar(req))) break;
    }
    fechar(fd);
}

std::string Skeleton::processar(Requisicao& req) {
    registrar("[skeleton] " + req.comando + " '" + req.nome + "' (id " + req.id + ")");
    {   // ID repetido? Devolve a mesma resposta sem executar de novo.
        std::lock_guard<std::mutex> trava(mutex_ids_);
        auto it = respostas_por_id_.find(req.id);
        if (it != respostas_por_id_.end()) {
            registrar("[skeleton] ID " + req.id + " repetido: ignorado (nada foi refeito)");
            return it->second;
        }
    }

    std::string resposta;
    bool alterou = false;
    try {
        const std::string& cmd = req.comando;
        if (cmd == "LISTAR") {
            resposta = resposta_ok(req.id, objeto_.listar());
        } else if (cmd == "BAIXAR") {
            resposta = resposta_ok(req.id, objeto_.baixar(req.nome));
        } else if (cmd == "ENVIAR") {
            req.versao = objeto_.enviar(req.nome, req.conteudo);  // nova versão
            resposta = resposta_ok(req.id, "");
            alterou = true;
        } else if (cmd == "REPLICAR") {
            // devolve 0 se a versão recebida for antiga (foi ignorada)
            alterou = objeto_.enviar(req.nome, req.conteudo, req.versao) > 0;
            resposta = resposta_ok(req.id, "");
        } else if (cmd == "REMOVER") {
            objeto_.remover(req.nome);
            resposta = resposta_ok(req.id, "");
            alterou = true;
        } else {
            throw std::runtime_error("COMANDO_INVALIDO");
        }
    } catch (const std::exception& e) {
        // Erros não são guardados: o cliente pode tentar de novo (ex.: OCUPADO).
        return resposta_erro(req.id, e.what());
    }

    if (req.comando != "LISTAR" && req.comando != "BAIXAR") {  // só alterações
        std::lock_guard<std::mutex> trava(mutex_ids_);
        respostas_por_id_[req.id] = resposta;
    }
    if (alterou && apos_alteracao) apos_alteracao(req);
    return resposta;
}
