// =====================================================================
// servidor_backup/main.cpp — SERVIDOR DE BACKUP (e nó P2P)
//
// Recebe replicações do principal (REPLICAR/REMOVER) e também atende
// clientes diretamente quando o principal cai. Ao ligar e a cada arquivo
// recebido, sincroniza com os vizinhos de peers.txt (peer-to-peer).
//
// Uso: backup <porta> <pasta> [arquivo_peers] [--sem-protecao]
// =====================================================================
#include <string>
#include <thread>
#include <vector>

#include "gerenciador_arquivos.hpp"
#include "p2p.hpp"
#include "rede.hpp"
#include "skeleton.hpp"

int main(int argc, char* argv[]) {
    bool protegido = true;
    std::vector<std::string> posicionais;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sem-protecao") protegido = false;
        else posicionais.push_back(arg);
    }
    int porta = 0;
    try {
        porta = std::stoi(posicionais.at(0));
        posicionais.at(1);
    } catch (...) {
        registrar("Uso: backup <porta> <pasta> [arquivo_peers] [--sem-protecao]");
        registrar("Ex.: ./bin/backup 6001 dados/backup1");
        return 1;
    }
    std::string pasta = posicionais[1];
    std::string arquivo_peers = posicionais.size() > 2 ? posicionais[2] : "servidor_backup/peers.txt";

    // Vizinhos = todos da lista, menos eu mesmo (mesma porta nesta máquina).
    std::vector<std::string> vizinhos;
    for (const auto& endereco : ler_lista_enderecos(arquivo_peers)) {
        std::string host;
        int p = 0;
        bool eu_mesmo = separar_endereco(endereco, host, p) && p == porta &&
                        (host == "localhost" || host == "127.0.0.1");
        if (!eu_mesmo) vizinhos.push_back(endereco);
    }

    registrar("=== SERVIDOR DE BACKUP === porta " + std::to_string(porta) + " | pasta: " + pasta +
              " | vizinhos P2P: " + std::to_string(vizinhos.size()));

    GerenciadorArquivos gerenciador(pasta, protegido);
    Skeleton skeleton(gerenciador);

    // Recebeu arquivo novo? Conversa com os vizinhos (em outra thread).
    skeleton.apos_alteracao = [&](const Requisicao& req) {
        if (req.comando == "REPLICAR" || req.comando == "ENVIAR")
            std::thread([&] { sincronizar_com_vizinhos(gerenciador, vizinhos); }).detach();
    };
    // Ao ligar: busca nos vizinhos o que perdeu enquanto estava desligado.
    std::thread([&] { sincronizar_com_vizinhos(gerenciador, vizinhos); }).detach();

    skeleton.servir(porta);  // nunca retorna
}
