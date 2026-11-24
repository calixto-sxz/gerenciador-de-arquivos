// =====================================================================
// servico/gerenciador_arquivos.hpp — o OBJETO REAL do serviço
//
// É ele que de fato grava e lê arquivos no disco. O skeleton chama seus
// métodos quando chega uma requisição. Usado pelo principal e pelos backups.
//
// Concorrência:
//  - um std::mutex POR ARQUIVO: duas escritas no mesmo arquivo não se misturam
//  - um SEMÁFORO contador (POSIX): no máximo N transferências ao mesmo tempo
//  - protegido = false (--sem-protecao) desliga os dois, para a demonstração
// =====================================================================
#pragma once
#include <semaphore.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class GerenciadorArquivos {
public:
    GerenciadorArquivos(const std::string& pasta, bool protegido, int max_transferencias = 3);

    std::string listar();                          // "nome versao\n" por arquivo
    std::string baixar(const std::string& nome);   // conteúdo do arquivo
    void remover(const std::string& nome);
    long versao(const std::string& nome);          // 0 se não existe

    // versao_recebida == 0: envio de cliente -> cria versão atual + 1.
    // versao_recebida  > 0: replicação -> só grava se for MAIOR que a atual.
    // Devolve a versão gravada, ou 0 se a versão recebida era antiga.
    long enviar(const std::string& nome, const std::string& conteudo, long versao_recebida = 0);

    // Ocupa / libera uma vaga do semáforo de transferências.
    void entrar_transferencia();
    void sair_transferencia();

private:
    void validar_nome(const std::string& nome);
    std::mutex& mutex_do_arquivo(const std::string& nome);
    void gravar_devagar(const std::string& caminho, const std::string& conteudo);
    void carregar_versoes();
    void salvar_versoes();  // chamar com mutex_mapa_ travado

    std::string pasta_;
    bool protegido_;
    sem_t semaforo_;                 // limita transferências simultâneas
    std::atomic<int> ativas_{0};     // só para mostrar no log
    std::mutex mutex_mapa_;          // protege os dois mapas abaixo
    std::map<std::string, long> versoes_;
    std::map<std::string, std::unique_ptr<std::mutex>> mutexes_;
};
