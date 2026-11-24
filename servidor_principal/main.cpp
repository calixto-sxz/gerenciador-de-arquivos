// =====================================================================
// servidor_principal/main.cpp — SERVIDOR PRINCIPAL
//
// Atende os clientes (uma thread por cliente, via Skeleton) e, depois de
// cada ENVIAR ou REMOVER, replica a operação para os backups listados em
// backups.txt. Backup fora do ar: tenta 3 vezes e registra a falha em log.
//
// Uso: principal [porta] [pasta] [arquivo_backups] [--sem-protecao]
// =====================================================================
#include <chrono>
#include <ctime>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "gerenciador_arquivos.hpp"
#include "proxy.hpp"
#include "rede.hpp"
#include "skeleton.hpp"

const int TENTATIVAS = 3;
const char* LOG_FALHAS = "dados/falhas_replicacao.log";

// Replica UMA operação para UM backup. Roda numa thread separada, então o
// cliente recebe a resposta sem esperar a replicação (replicação assíncrona).
void replicar_para(const std::string& backup, Requisicao req) {
    for (int tentativa = 1; tentativa <= TENTATIVAS; ++tentativa) {
        try {
            GerenciadorProxy proxy({backup});   // o principal vira CLIENTE do backup
            proxy.fixar_id("rep-" + req.id);    // mesmo ID em toda tentativa
            if (req.comando == "REMOVER") proxy.remover(req.nome);
            else proxy.replicar(req.nome, req.conteudo, req.versao);
            registrar("[replicacao] " + req.comando + " " + req.nome + " -> " + backup + " OK");
            return;
        } catch (const std::exception& e) {
            std::string motivo = e.what();
            if (motivo == "ARQUIVO_INEXISTENTE") return;  // backup já não tinha: tudo certo
            registrar("[replicacao] falha em " + backup + " (tentativa " + std::to_string(tentativa) +
                      "/" + std::to_string(TENTATIVAS) + "): " + motivo);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    // Falha PARCIAL: o sistema continua funcionando, só fica registrado.
    registrar("[replicacao] DESISTI de " + backup + " para " + req.nome + " (veja " + LOG_FALHAS + ")");
    std::time_t agora = std::time(nullptr);
    char data[32];
    std::strftime(data, sizeof(data), "%d/%m/%Y %H:%M:%S", std::localtime(&agora));
    std::ofstream log(LOG_FALHAS, std::ios::app);
    log << data << " " << req.comando << " " << req.nome << " v" << req.versao
        << " -> " << backup << " FALHOU\n";
}

int main(int argc, char* argv[]) {
    bool protegido = true;
    std::vector<std::string> posicionais;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sem-protecao") protegido = false;
        else posicionais.push_back(arg);
    }
    int porta = 5000;
    try {
        if (posicionais.size() > 0) porta = std::stoi(posicionais[0]);
    } catch (...) {
        registrar("Uso: principal [porta] [pasta] [arquivo_backups] [--sem-protecao]");
        return 1;
    }
    std::string pasta = posicionais.size() > 1 ? posicionais[1] : "dados/principal";
    std::string arquivo_backups = posicionais.size() > 2 ? posicionais[2]
                                                         : "servidor_principal/backups.txt";
    std::vector<std::string> backups = ler_lista_enderecos(arquivo_backups);

    registrar("=== SERVIDOR PRINCIPAL === pasta: " + pasta + " | backups: " +
              std::to_string(backups.size()) + " | proteção (mutex + semáforo): " +
              (protegido ? "LIGADA" : "DESLIGADA (--sem-protecao)"));

    GerenciadorArquivos gerenciador(pasta, protegido);
    Skeleton skeleton(gerenciador);
    skeleton.apos_alteracao = [&backups](const Requisicao& req) {
        if (req.comando != "ENVIAR" && req.comando != "REMOVER") return;
        for (const auto& backup : backups) std::thread(replicar_para, backup, req).detach();
    };
    skeleton.servir(porta);  // nunca retorna
}
