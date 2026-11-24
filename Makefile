# Makefile simples: make principal | make backup | make cliente | make (tudo)
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -Imiddleware -Iservico

TRANSPORTE = middleware/rede.cpp middleware/protocolo.cpp
PROXY      = $(TRANSPORTE) middleware/proxy.cpp
SERVIDOR   = $(PROXY) middleware/skeleton.cpp servico/gerenciador_arquivos.cpp
CABECALHOS = $(wildcard middleware/*.hpp servico/*.hpp servidor_backup/*.hpp)

.PHONY: all principal backup cliente clean
all: principal backup cliente

principal: bin/principal
backup:    bin/backup
cliente:   bin/cliente

bin/principal: servidor_principal/main.cpp $(SERVIDOR) $(CABECALHOS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

bin/backup: servidor_backup/main.cpp servidor_backup/p2p.cpp $(SERVIDOR) $(CABECALHOS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

bin/cliente: cliente/main.cpp $(PROXY) $(CABECALHOS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

clean:
	rm -rf bin
