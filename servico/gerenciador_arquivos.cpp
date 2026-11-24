// =====================================================================
// servico/gerenciador_arquivos.cpp — leitura/escrita com mutex e semáforo
// =====================================================================
#include "gerenciador_arquivos.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "rede.hpp"  // registrar()

namespace fs = std::filesystem;

const size_t PEDACO = 16;   // escreve 16 bytes por vez...
const int PAUSA_MS = 5;     // ...com 5 ms de pausa: deixa a corrida VISÍVEL

// Guarda "RAII": ocupa uma vaga no construtor e libera no destrutor.
struct Transferencia {
    GerenciadorArquivos& g;
    explicit Transferencia(GerenciadorArquivos& ger) : g(ger) { g.entrar_transferencia(); }
    ~Transferencia() { g.sair_transferencia(); }
};

GerenciadorArquivos::GerenciadorArquivos(const std::string& pasta, bool protegido, int max)
    : pasta_(pasta), protegido_(protegido) {
    fs::create_directories(pasta_);
    sem_init(&semaforo_, 0, max);  // semáforo começa com 'max' vagas
    carregar_versoes();
}

void GerenciadorArquivos::entrar_transferencia() {
    if (protegido_) {  // espera até 5 s por uma vaga; senão, SERVIDOR_OCUPADO
        timespec limite;
        clock_gettime(CLOCK_REALTIME, &limite);
        limite.tv_sec += 5;
        if (sem_timedwait(&semaforo_, &limite) != 0) throw std::runtime_error("SERVIDOR_OCUPADO");
    }
    registrar("[semaforo] transferência iniciada (ativas: " + std::to_string(++ativas_) + ")");
}

void GerenciadorArquivos::sair_transferencia() {
    --ativas_;
    if (protegido_) sem_post(&semaforo_);
}

void GerenciadorArquivos::validar_nome(const std::string& nome) {
    // Sem subpastas, sem ';' (separador do protocolo) e sem arquivos ocultos.
    if (nome.empty() || nome.size() > 100 || nome[0] == '.' ||
        nome.find_first_of("/;\n") != std::string::npos)
        throw std::runtime_error("NOME_INVALIDO");
}

std::mutex& GerenciadorArquivos::mutex_do_arquivo(const std::string& nome) {
    std::lock_guard<std::mutex> trava(mutex_mapa_);
    auto& m = mutexes_[nome];
    if (!m) m = std::make_unique<std::mutex>();  // cria na primeira vez
    return *m;
}

// Escrita LENTA e em pedaços: (1) esvazia o arquivo, (2) acrescenta pedaço
// por pedaço. Sem mutex, pedaços de clientes diferentes se intercalam.
void GerenciadorArquivos::gravar_devagar(const std::string& caminho, const std::string& conteudo) {
    std::ofstream(caminho, std::ios::binary | std::ios::trunc);
    std::ofstream arquivo(caminho, std::ios::binary | std::ios::app);
    for (size_t i = 0; i < conteudo.size(); i += PEDACO) {
        arquivo.write(conteudo.data() + i, std::min(PEDACO, conteudo.size() - i));
        arquivo.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(PAUSA_MS));
    }
}

long GerenciadorArquivos::enviar(const std::string& nome, const std::string& conteudo,
                                 long versao_recebida) {
    validar_nome(nome);
    Transferencia vaga(*this);                         // semáforo
    std::unique_lock<std::mutex> trava(mutex_do_arquivo(nome), std::defer_lock);
    if (protegido_) trava.lock();                      // mutex do arquivo

    long nova = versao(nome) + 1;
    if (versao_recebida > 0) {
        if (versao_recebida < nova) {
            registrar("[arquivo] " + nome + " v" + std::to_string(versao_recebida) +
                      " ignorada (já tenho v" + std::to_string(nova - 1) + ")");
            return 0;
        }
        nova = versao_recebida;  // na sincronização vence a versão MAIOR
    }
    gravar_devagar(pasta_ + "/" + nome, conteudo);
    {
        std::lock_guard<std::mutex> t(mutex_mapa_);
        versoes_[nome] = nova;
        salvar_versoes();
    }
    registrar("[arquivo] " + nome + " gravado (versão " + std::to_string(nova) + ", " +
              std::to_string(conteudo.size()) + " bytes)");
    return nova;
}

std::string GerenciadorArquivos::baixar(const std::string& nome) {
    validar_nome(nome);
    Transferencia vaga(*this);
    std::unique_lock<std::mutex> trava(mutex_do_arquivo(nome), std::defer_lock);
    if (protegido_) trava.lock();  // não lê um arquivo pela metade
    std::ifstream arquivo(pasta_ + "/" + nome, std::ios::binary);
    if (!arquivo) throw std::runtime_error("ARQUIVO_INEXISTENTE");
    std::ostringstream conteudo;
    conteudo << arquivo.rdbuf();
    return conteudo.str();
}

void GerenciadorArquivos::remover(const std::string& nome) {
    validar_nome(nome);
    std::unique_lock<std::mutex> trava(mutex_do_arquivo(nome), std::defer_lock);
    if (protegido_) trava.lock();
    if (!fs::remove(pasta_ + "/" + nome)) throw std::runtime_error("ARQUIVO_INEXISTENTE");
    std::lock_guard<std::mutex> t(mutex_mapa_);
    versoes_.erase(nome);
    salvar_versoes();
    registrar("[arquivo] " + nome + " removido");
}

long GerenciadorArquivos::versao(const std::string& nome) {
    std::lock_guard<std::mutex> trava(mutex_mapa_);
    auto it = versoes_.find(nome);
    return it == versoes_.end() ? 0 : it->second;
}

std::string GerenciadorArquivos::listar() {
    std::map<std::string, long> arquivos;  // map: sai em ordem alfabética
    for (const auto& item : fs::directory_iterator(pasta_)) {
        std::string nome = item.path().filename().string();
        if (item.is_regular_file() && nome[0] != '.') arquivos[nome] = versao(nome);
    }
    std::string texto;
    for (const auto& [nome, v] : arquivos) texto += nome + " " + std::to_string(v) + "\n";
    return texto;
}

// As versões ficam no arquivo oculto ".versoes" (uma linha "nome versao").
void GerenciadorArquivos::carregar_versoes() {
    std::ifstream entrada(pasta_ + "/.versoes");
    std::string nome;
    long v;
    while (entrada >> nome >> v) versoes_[nome] = v;
}

void GerenciadorArquivos::salvar_versoes() {
    std::ofstream saida(pasta_ + "/.versoes", std::ios::trunc);
    for (const auto& [nome, v] : versoes_) saida << nome << " " << v << "\n";
}
