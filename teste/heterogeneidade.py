#!/usr/bin/env python3
"""
teste/heterogeneidade.py — DUAS LINGUAGENS NO MESMO CÓDIGO

Este script em Python usa o programa em C++ como um COMANDO (subprocess):
  1. o Python manda o cliente C++ enviar um arquivo; o Python baixa e lê;
  2. o Python envia um arquivo; manda o cliente C++ baixar e ler.
Os dois se entendem porque seguem o mesmo protocolo (docs/protocolo.md).

Pré-requisito: make  e o servidor rodando:  ./bin/principal
Uso: python3 teste/heterogeneidade.py
"""
import os
import subprocess
import sys

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(RAIZ, "cliente_python"))
from cliente import chamar  # noqa: E402  (a função do nosso cliente Python)

os.chdir(RAIZ)
SERVIDOR = ["localhost:5000"]


def cliente_cpp(*argumentos):
    """Executa o cliente C++ como um comando e devolve o que ele imprimiu."""
    comando = ["./bin/cliente", "-s", SERVIDOR[0], *argumentos]
    print("   $ " + " ".join(comando))
    resultado = subprocess.run(comando, capture_output=True, text=True)
    if resultado.returncode != 0:
        sys.exit("   Erro no C++: " + resultado.stderr.strip())
    return resultado.stdout


print("1) C++ envia, Python lê")
os.makedirs("dados", exist_ok=True)
with open("dados/do_cpp.txt", "w") as arquivo:
    arquivo.write("Escrito pelo C++, lido pelo Python!\n")
print("   C++ diz: " + cliente_cpp("enviar", "dados/do_cpp.txt").strip())
ok, dados = chamar(SERVIDOR, "BAIXAR", "do_cpp.txt")
print("   Python leu: " + dados.decode().strip())

print("\n2) Python envia, C++ lê")
ok, _ = chamar(SERVIDOR, "ENVIAR", "do_python.txt", b"Escrito pelo Python, lido pelo C++!\n")
print("   Python enviou do_python.txt")
print("   C++ leu: " + cliente_cpp("baixar", "do_python.txt").strip())
