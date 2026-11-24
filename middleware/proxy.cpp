// =====================================================================
// middleware/proxy.cpp — implementação do stub do cliente
// =====================================================================
#include "proxy.hpp"

#include <cstdio>
#include <random>

#include "rede.hpp"

GerenciadorProxy::GerenciadorProxy(std::vector<std::string> servidores)
    : servidores_(std::move(servidores)) {}

// ID único de requisição: 16 dígitos hexadecimais aleatórios.
static std::string novo_id() {
    static thread_local std::mt19937_64 gerador(std::random_device{}());
    char texto[17];
    std::snprintf(texto, sizeof(texto), "%016llx", (unsigned long long)gerador());
    return texto;
}

Resposta GerenciadorProxy::chamar(Requisicao req) {
    req.id = id_fixo_.empty() ? novo_id() : id_fixo_;
    std::string pacote = montar_requisicao(req);  // 1) empacota

    for (const auto& servidor : servidores_) {
        std::string host;
        int porta = 0;
        if (!separar_endereco(servidor, host, porta)) {
            registrar("[proxy] endereço inválido: " + servidor);
            continue;
        }
        int fd = conectar(host, porta);  // 2) abre a conexão
        if (fd < 0) {
            registrar("[proxy] " + servidor + " fora do ar (conexão recusada)");
            continue;  // falha parcial: tenta o próximo servidor
        }
        Resposta resp;
        bool ok = enviar_tudo(fd, pacote) && ler_resposta(fd, resp);  // 3) envia e espera
        fechar(fd);
        if (ok) {
            servidor_usado_ = servidor;
            return resp;  // 4) devolve o resultado ao chamador
        }
        registrar("[proxy] " + servidor + " não respondeu");
    }
    throw std::runtime_error("nenhum servidor disponível (conexão recusada)");
}

// Se o servidor respondeu ERRO, vira uma exceção para quem chamou.
static void verificar(const Resposta& resp) {
    if (!resp.ok) throw ErroRemoto(resp.motivo);
}

std::string GerenciadorProxy::listar() {
    Requisicao req;
    req.comando = "LISTAR";
    Resposta resp = chamar(req);
    verificar(resp);
    return resp.conteudo;
}

void GerenciadorProxy::enviar(const std::string& nome, const std::string& conteudo) {
    Requisicao req;
    req.comando = "ENVIAR";
    req.nome = nome;
    req.conteudo = conteudo;
    verificar(chamar(req));
}

std::string GerenciadorProxy::baixar(const std::string& nome) {
    Requisicao req;
    req.comando = "BAIXAR";
    req.nome = nome;
    Resposta resp = chamar(req);
    verificar(resp);
    return resp.conteudo;
}

void GerenciadorProxy::remover(const std::string& nome) {
    Requisicao req;
    req.comando = "REMOVER";
    req.nome = nome;
    verificar(chamar(req));
}

void GerenciadorProxy::replicar(const std::string& nome, const std::string& conteudo, long versao) {
    Requisicao req;
    req.comando = "REPLICAR";
    req.nome = nome;
    req.conteudo = conteudo;
    req.versao = versao;
    verificar(chamar(req));
}
