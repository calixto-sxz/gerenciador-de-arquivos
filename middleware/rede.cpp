// =====================================================================
// middleware/rede.cpp — tudo o que mexe com socket fica AQUI
// =====================================================================
#include "rede.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>

// Abre uma conexão TCP com host:porta. Devolve o descritor ou -1.
int conectar(const std::string& host, int porta) {
    addrinfo dicas{};
    dicas.ai_family = AF_INET;        // IPv4
    dicas.ai_socktype = SOCK_STREAM;  // TCP
    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(porta).c_str(), &dicas, &res) != 0) return -1;
    int fd = socket(res->ai_family, res->ai_socktype, 0);
    if (fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        close(fd);  // conexão recusada: ninguém escutando nessa porta
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd >= 0) {
        // Se o servidor travar, não esperamos para sempre (latência / falha).
        timeval limite{15, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &limite, sizeof(limite));
    }
    return fd;
}

// Cria o socket que fica esperando conexões na porta indicada.
int escutar(int porta) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int sim = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sim, sizeof(sim));
    sockaddr_in endereco{};
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(porta);
    if (bind(fd, (sockaddr*)&endereco, sizeof(endereco)) != 0 || listen(fd, 16) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

// Bloqueia até um cliente conectar; devolve o socket dessa conexão.
int aceitar(int fd_escuta) { return accept(fd_escuta, nullptr, nullptr); }

void fechar(int fd) { close(fd); }

// send() pode enviar só parte dos dados; repetimos até acabar.
bool enviar_tudo(int fd, const std::string& dados) {
    size_t enviado = 0;
    while (enviado < dados.size()) {
        ssize_t n = send(fd, dados.data() + enviado, dados.size() - enviado, MSG_NOSIGNAL);
        if (n <= 0) return false;
        enviado += n;
    }
    return true;
}

// Lê byte a byte até o '\n' (simples e suficiente para o cabeçalho).
bool ler_linha(int fd, std::string& linha) {
    linha.clear();
    char c;
    while (linha.size() < 4096) {
        if (recv(fd, &c, 1, 0) != 1) return false;
        if (c == '\n') {
            if (!linha.empty() && linha.back() == '\r') linha.pop_back();
            return true;
        }
        linha += c;
    }
    return false;  // linha grande demais: cabeçalho inválido
}

// Lê exatamente n bytes (o conteúdo do arquivo depois do cabeçalho).
bool ler_bytes(int fd, size_t n, std::string& dados) {
    dados.assign(n, '\0');
    size_t lido = 0;
    while (lido < n) {
        ssize_t r = recv(fd, &dados[lido], n - lido, 0);
        if (r <= 0) return false;
        lido += r;
    }
    return true;
}

std::vector<std::string> separar(const std::string& texto, char sep) {
    std::vector<std::string> partes(1);
    for (char c : texto) {
        if (c == sep) partes.emplace_back();
        else partes.back() += c;
    }
    return partes;
}

// "localhost:6001" -> host = "localhost", porta = 6001
bool separar_endereco(const std::string& endereco, std::string& host, int& porta) {
    auto pos = endereco.rfind(':');
    if (pos == std::string::npos) return false;
    host = endereco.substr(0, pos);
    try { porta = std::stoi(endereco.substr(pos + 1)); } catch (...) { return false; }
    return true;
}

// Lê um arquivo com um endereço host:porta por linha (ignora vazias e '#').
std::vector<std::string> ler_lista_enderecos(const std::string& arquivo) {
    std::vector<std::string> lista;
    std::ifstream entrada(arquivo);
    std::string linha;
    while (std::getline(entrada, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();
        if (!linha.empty() && linha[0] != '#') lista.push_back(linha);
    }
    return lista;
}

void registrar(const std::string& mensagem) {
    static std::mutex mutex_log;  // várias threads escrevem no log
    std::lock_guard<std::mutex> trava(mutex_log);
    std::time_t agora = std::time(nullptr);
    char hora[16];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", std::localtime(&agora));
    std::cerr << "[" << hora << "] " << mensagem << std::endl;
}
