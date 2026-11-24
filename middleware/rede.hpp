// =====================================================================
// middleware/rede.hpp — camada de TRANSPORTE do middleware
// Esconde os detalhes de sockets POSIX atrás de funções simples.
// Nenhum outro componente (cliente, serviço) chama socket() diretamente.
// =====================================================================
#pragma once
#include <string>
#include <vector>

int  conectar(const std::string& host, int porta);   // devolve o socket ou -1
int  escutar(int porta);                              // socket de escuta ou -1
int  aceitar(int fd_escuta);                          // espera o próximo cliente
void fechar(int fd);

bool enviar_tudo(int fd, const std::string& dados);        // envia todos os bytes
bool ler_linha(int fd, std::string& linha);                // lê até o '\n'
bool ler_bytes(int fd, size_t n, std::string& dados);      // lê exatamente n bytes

// Utilitários de texto
std::vector<std::string> separar(const std::string& texto, char sep);
bool separar_endereco(const std::string& endereco, std::string& host, int& porta);
std::vector<std::string> ler_lista_enderecos(const std::string& arquivo);

// Log com horário, seguro para várias threads ao mesmo tempo
void registrar(const std::string& mensagem);
