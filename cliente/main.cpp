// =====================================================================
// cliente/main.cpp — CLIENTE de linha de comando (C++)
//
// Repare: aqui não existe nenhum socket! O cliente só chama métodos do
// GerenciadorProxy como se o objeto fosse local. O middleware faz o resto.
// =====================================================================
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "proxy.hpp"

static void uso() {
    std::cerr << "Uso: cliente [-s host:porta]... <comando> [argumentos]\n"
                 "Comandos:\n"
                 "  listar                              lista arquivos e versões\n"
                 "  enviar <arquivo_local> [nome]       envia um arquivo\n"
                 "  baixar <nome> [arquivo_local]       baixa (mostra na tela se não der destino)\n"
                 "  remover <nome>                      remove um arquivo\n"
                 "  duplicar <arquivo_local>            envia 2x com o MESMO ID (teste)\n"
                 "Sem -s, tenta: localhost:5000 (principal), depois os backups 6001 e 6002.\n";
}

static std::string ler_arquivo_local(const std::string& caminho) {
    std::ifstream arquivo(caminho, std::ios::binary);
    if (!arquivo) throw std::runtime_error("não consegui abrir o arquivo local '" + caminho + "'");
    std::ostringstream conteudo;
    conteudo << arquivo.rdbuf();
    return conteudo.str();
}

static std::string nome_de(const std::vector<std::string>& args) {
    return args.size() > 2 ? args[2] : std::filesystem::path(args[1]).filename().string();
}

int main(int argc, char* argv[]) {
    std::vector<std::string> servidores, args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" && i + 1 < argc) servidores.push_back(argv[++i]);
        else args.push_back(arg);
    }
    if (servidores.empty()) servidores = {"localhost:5000", "localhost:6001", "localhost:6002"};
    if (args.empty()) {
        uso();
        return 1;
    }

    GerenciadorProxy gerenciador(servidores);  // o "objeto remoto"
    const std::string& cmd = args[0];
    try {
        if (cmd == "listar") {
            std::istringstream lista(gerenciador.listar());
            std::string nome;
            long versao;
            int total = 0;
            while (lista >> nome >> versao) {
                std::printf("  %-30s versão %ld\n", nome.c_str(), versao);
                ++total;
            }
            if (total == 0) std::cout << "  (nenhum arquivo)\n";
        } else if (cmd == "enviar" && args.size() >= 2) {
            gerenciador.enviar(nome_de(args), ler_arquivo_local(args[1]));
            std::cout << "Arquivo '" << nome_de(args) << "' enviado.\n";
        } else if (cmd == "baixar" && args.size() >= 2) {
            std::string conteudo = gerenciador.baixar(args[1]);
            if (args.size() > 2) {
                std::ofstream(args[2], std::ios::binary) << conteudo;
                std::cout << "Salvo em '" << args[2] << "' (" << conteudo.size() << " bytes).\n";
            } else {
                std::cout << conteudo;
            }
        } else if (cmd == "remover" && args.size() >= 2) {
            gerenciador.remover(args[1]);
            std::cout << "Arquivo '" << args[1] << "' removido.\n";
        } else if (cmd == "duplicar" && args.size() >= 2) {
            // Simula um cliente que reenviou a requisição (ex.: achou que perdeu a resposta)
            std::string conteudo = ler_arquivo_local(args[1]);
            gerenciador.fixar_id("dup" + std::to_string(std::time(nullptr)));
            gerenciador.enviar(nome_de(args), conteudo);
            gerenciador.enviar(nome_de(args), conteudo);  // mesmo ID: o servidor ignora
            std::cout << "Enviado 2 vezes com o mesmo ID. Rode 'listar': a versão subiu só 1.\n";
        } else {
            uso();
            return 1;
        }
        std::cerr << "(atendido por " << gerenciador.servidor_usado() << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
