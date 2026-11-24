// =====================================================================
// servidor_backup/p2p.hpp — sincronização PEER-TO-PEER entre backups
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "gerenciador_arquivos.hpp"

// Compara arquivos e versões com cada vizinho e troca direto o que falta.
void sincronizar_com_vizinhos(GerenciadorArquivos& meu, const std::vector<std::string>& vizinhos);
