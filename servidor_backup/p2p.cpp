// =====================================================================
// servidor_backup/p2p.cpp — cada backup é CLIENTE e SERVIDOR ao mesmo tempo
//
// Para cada vizinho:
//   1. pede a lista dele (LISTAR) -> "nome versao"
//   2. o que ele tem com versão MAIOR, eu baixo (BAIXAR)
//   3. o que eu tenho com versão MAIOR (ou ele não tem), eu envio (REPLICAR)
// Não passa pelo servidor principal.
// =====================================================================
#include "p2p.hpp"

#include <map>
#include <mutex>
#include <sstream>

#include "proxy.hpp"
#include "rede.hpp"

// Converte o texto do LISTAR em um mapa nome -> versão.
static std::map<std::string, long> ler_lista(const std::string& texto) {
    std::map<std::string, long> lista;
    std::istringstream entrada(texto);
    std::string nome;
    long versao;
    while (entrada >> nome >> versao) lista[nome] = versao;
    return lista;
}

static void sincronizar_com(GerenciadorArquivos& meu, const std::string& vizinho) {
    GerenciadorProxy proxy({vizinho});
    auto deles = ler_lista(proxy.listar());
    auto meus = ler_lista(meu.listar());

    for (const auto& [nome, v] : deles) {  // puxa o que é mais novo lá
        if (v > meus[nome]) {
            meu.enviar(nome, proxy.baixar(nome), v);
            registrar("[p2p] recebi " + nome + " v" + std::to_string(v) + " de " + vizinho);
        }
    }
    for (const auto& [nome, v] : meus) {  // empurra o que é mais novo aqui
        if (v > 0 && v > deles[nome]) {
            proxy.replicar(nome, meu.baixar(nome), v);
            registrar("[p2p] enviei " + nome + " v" + std::to_string(v) + " para " + vizinho);
        }
    }
}

void sincronizar_com_vizinhos(GerenciadorArquivos& meu, const std::vector<std::string>& vizinhos) {
    static std::mutex uma_por_vez;  // evita duas sincronizações simultâneas no mesmo nó
    std::lock_guard<std::mutex> trava(uma_por_vez);
    for (const auto& vizinho : vizinhos) {
        try {
            sincronizar_com(meu, vizinho);
        } catch (const std::exception& e) {
            registrar("[p2p] vizinho " + vizinho + " indisponível: " + e.what());
        }
    }
}
