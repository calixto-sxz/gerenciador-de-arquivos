#!/usr/bin/env python3
"""
cliente_python/cliente.py — segundo cliente, escrito em PYTHON.

Demonstra HETEROGENEIDADE: linguagem diferente do servidor (C++), mas
fala o MESMO protocolo texto. Não usa nenhuma biblioteca além da padrão.

Uso: python3 cliente_python/cliente.py [-s host:porta]... <comando> [args]
Comandos: listar | enviar <arquivo> [nome] | baixar <nome> [destino] | remover <nome>
"""
import os
import socket
import sys
import uuid

SERVIDORES_PADRAO = ["localhost:5000", "localhost:6001", "localhost:6002"]


def ler_linha(sock):
    """Lê bytes até o '\\n' (o cabeçalho da resposta)."""
    linha = b""
    while not linha.endswith(b"\n"):
        byte = sock.recv(1)
        if not byte:
            raise ConnectionError("conexão fechada pelo servidor")
        linha += byte
    return linha.decode().strip()


def ler_bytes(sock, n):
    """Lê exatamente n bytes (o conteúdo)."""
    dados = b""
    while len(dados) < n:
        pedaco = sock.recv(n - len(dados))
        if not pedaco:
            raise ConnectionError("conexão fechada pelo servidor")
        dados += pedaco
    return dados


def chamar(servidores, comando, nome="", conteudo=b""):
    """Monta 'ID;COMANDO;nome;tamanho\\n' + conteúdo e envia. Devolve (ok, dados)."""
    id_req = uuid.uuid4().hex[:16]  # ID único de requisição
    pacote = f"{id_req};{comando};{nome};{len(conteudo)}\n".encode() + conteudo
    for servidor in servidores:
        host, porta = servidor.rsplit(":", 1)
        try:
            with socket.create_connection((host, int(porta)), timeout=15) as sock:
                sock.sendall(pacote)
                campos = ler_linha(sock).split(";")  # ID;OK;tamanho ou ID;ERRO;motivo
                print(f"(atendido por {servidor})", file=sys.stderr)
                if campos[1] == "OK":
                    return True, ler_bytes(sock, int(campos[2]))
                return False, campos[2]
        except OSError as erro:
            print(f"[python] {servidor} fora do ar ({erro.strerror or erro})", file=sys.stderr)
    sys.exit("Erro: nenhum servidor disponível (conexão recusada)")


def main():
    args = sys.argv[1:]
    servidores = []
    while len(args) >= 2 and args[0] == "-s":
        servidores.append(args[1])
        args = args[2:]
    servidores = servidores or SERVIDORES_PADRAO
    if not args:
        sys.exit(__doc__)

    comando = args[0]
    if comando == "listar":
        ok, dados = chamar(servidores, "LISTAR")
        if ok:
            linhas = dados.decode().splitlines()
            for linha in linhas:
                nome, versao = linha.split()
                print(f"  {nome:<30} versão {versao}")
            if not linhas:
                print("  (nenhum arquivo)")
    elif comando == "enviar" and len(args) >= 2:
        nome = args[2] if len(args) > 2 else os.path.basename(args[1])
        try:
            with open(args[1], "rb") as arquivo:
                conteudo = arquivo.read()
        except OSError:
            sys.exit(f"Erro: não consegui abrir o arquivo local '{args[1]}'")
        ok, dados = chamar(servidores, "ENVIAR", nome, conteudo)
        if ok:
            print(f"Arquivo '{nome}' enviado (pelo cliente Python).")
    elif comando == "baixar" and len(args) >= 2:
        ok, dados = chamar(servidores, "BAIXAR", args[1])
        if ok and len(args) > 2:
            with open(args[2], "wb") as arquivo:
                arquivo.write(dados)
            print(f"Salvo em '{args[2]}' ({len(dados)} bytes).")
        elif ok:
            sys.stdout.write(dados.decode(errors="replace"))
    elif comando == "remover" and len(args) >= 2:
        ok, dados = chamar(servidores, "REMOVER", args[1])
        if ok:
            print(f"Arquivo '{args[1]}' removido.")
    else:
        sys.exit(__doc__)

    if not ok:
        sys.exit(f"Erro: {dados}")


if __name__ == "__main__":
    main()
