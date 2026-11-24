// =====================================================================
// middleware/protocolo.cpp — transforma structs em texto e vice-versa
// =====================================================================
#include "protocolo.hpp"
#include "rede.hpp"

std::string montar_requisicao(const Requisicao& req) {
    std::string cabecalho = req.id + ";" + req.comando + ";" + req.nome + ";" +
                            std::to_string(req.conteudo.size());
    if (req.comando == "REPLICAR") cabecalho += ";" + std::to_string(req.versao);
    return cabecalho + "\n" + req.conteudo;
}

std::string ler_requisicao(int fd, Requisicao& req) {
    std::string linha;
    if (!ler_linha(fd, linha)) return "CONEXAO_FECHADA";
    auto campos = separar(linha, ';');
    if (campos.size() < 4 || campos[0].empty()) return "FORMATO_INVALIDO";
    req.id = campos[0];
    req.comando = campos[1];
    req.nome = campos[2];
    long tamanho = 0;
    try {
        tamanho = std::stol(campos[3]);
        if (campos.size() > 4) req.versao = std::stol(campos[4]);
    } catch (...) {
        return "FORMATO_INVALIDO";
    }
    if (tamanho < 0) return "FORMATO_INVALIDO";
    if (tamanho > TAMANHO_MAXIMO) return "ARQUIVO_GRANDE";
    if (!ler_bytes(fd, tamanho, req.conteudo)) return "CONEXAO_FECHADA";
    return "";
}

std::string resposta_ok(const std::string& id, const std::string& conteudo) {
    return id + ";OK;" + std::to_string(conteudo.size()) + "\n" + conteudo;
}

std::string resposta_erro(const std::string& id, const std::string& motivo) {
    return id + ";ERRO;" + motivo + "\n";
}

bool ler_resposta(int fd, Resposta& resp) {
    std::string linha;
    if (!ler_linha(fd, linha)) return false;
    auto campos = separar(linha, ';');
    if (campos.size() < 3) return false;
    resp.id = campos[0];
    resp.ok = (campos[1] == "OK");
    if (!resp.ok) {
        resp.motivo = campos[2];
        return true;
    }
    try {
        return ler_bytes(fd, std::stoul(campos[2]), resp.conteudo);
    } catch (...) {
        return false;
    }
}
