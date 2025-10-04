#!/bin/bash

echo "=== Teste do Servidor de Chat ==="
echo "Compilando..."
make

echo -e "\nIniciando servidor em background..."
./bin/servidor_chat &
SERVER_PID=$!

sleep 2

echo -e "\nIniciando testes com múltiplos clientes..."
echo "Nota: Digite mensagens e 'SAIR' para finalizar cada cliente"

# Função para simular um cliente automático
run_automated_client() {
    local client_num=$1
    local nickname="Cliente$client_num"
    
    # Criar um arquivo temporário com os comandos
    cat > /tmp/client_${client_num}_commands.txt << EOF
${nickname}
Olá pessoal! Sou o cliente ${client_num}
Como vocês estão?
SAIR
EOF
    
    # Executar cliente com input redirecionado
    ./bin/cliente_chat < /tmp/client_${client_num}_commands.txt
    
    # Limpar
    rm -f /tmp/client_${client_num}_commands.txt
}

# Teste com 3 clientes automáticos em background
echo -e "\n--- Teste Automático com Clientes Reais ---"
for i in {1..3}; do
    echo "Iniciando cliente automático $i..."
    run_automated_client $i &
    CLIENT_PIDS[$i]=$!
    sleep 1
done

echo -e "\nAguardando clientes automáticos..."
sleep 8

# Aguardar clientes automáticos terminarem
for i in {1..3}; do
    if [ -n "${CLIENT_PIDS[$i]}" ]; then
        wait ${CLIENT_PIDS[$i]} 2>/dev/null
    fi
done

echo -e "\n--- Iniciando Clientes Interativos ---"
# Array para armazenar PIDs dos clientes interativos
INTERACTIVE_PIDS=()

# Iniciar clientes interativos em terminais separados
for i in {4..5}; do
    echo -e "\n--- Iniciando Cliente Interativo $i ---"
    
    # Verificar qual terminal está disponível
    if command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="Cliente $i" -- ./bin/cliente_chat &
    elif command -v xterm &> /dev/null; then
        xterm -title "Cliente $i" -e ./bin/cliente_chat &
    elif command -v konsole &> /dev/null; then
        konsole --new-tab -p tabtitle="Cliente $i" -e ./bin/cliente_chat &
    else
        echo "Nenhum terminal gráfico encontrado. Executando em background."
        ./bin/cliente_chat &
    fi
    
    INTERACTIVE_PIDS+=($!)
    sleep 2
done

echo -e "\n=== Teste em Execução ==="
echo "Verifique:"
echo "1. Console do servidor - mostrando todas as mensagens"
echo "2. Terminais dos clientes - para comunicação interativa"
echo "3. Arquivo servidor_chat.log - logs completos"
echo -e "\nPressione Enter para finalizar todos os processos..."
read

echo -e "\nFinalizando servidor e clientes..."
# Finalizar servidor
kill $SERVER_PID 2>/dev/null && echo "Servidor finalizado."

# Finalizar clientes interativos
for pid in "${INTERACTIVE_PIDS[@]}"; do
    kill $pid 2>/dev/null
done

# Garantir que todos os processos relacionados sejam finalizados
pkill -f "./bin/cliente_chat" 2>/dev/null
pkill -f "./bin/servidor_chat" 2>/dev/null

echo "Teste finalizado."