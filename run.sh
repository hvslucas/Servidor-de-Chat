#!/bin/bash
set -e

BIN_DIR="./bin"

while true; do
  echo -e "\n \033[96m1\033[0m - Compilar (make)"
  echo -e " \033[96m2\033[0m - Executar log_test (padrão: 3 threads x 10 msgs)"
  echo -e " \033[96m3\033[0m - Executar log_test customizado (args)"
  echo -e " \033[96m4\033[0m - Executar servidor"
  echo -e " \033[96m5\033[0m - Executar cliente"
  echo -e " \033[96m6\033[0m - Limpar"
  echo -e " \033[96m0\033[0m - Sair"
  read -p "Escolha: " opt

  case "$opt" in
    1)
      make all
      clear
      ;;
    2)
      $BIN_DIR/log_test
      exit 0
      ;;
    3)
      read -p "Threads: " t
      read -p "Msgs: " m
      $BIN_DIR/log_test "$t" "$m"
      exit 0
      ;;
    4)
      $BIN_DIR/servidor_chat
      exit 0
      ;;
    5)
      $BIN_DIR/cliente_chat
      exit 0
      ;;
    6)
      make clean
      clear
      ;;
    0)
      exit 0
      ;;
    *)
      echo "Opção inválida"
      ;;
  esac
done
