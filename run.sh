#!/bin/bash
set -e

# Loop principal para exibir o menu novamente após a execução das opções 1 e 4
while true; do
  echo -e " \033[96m1\033[0m - Compilar (make)"
  echo -e " \033[96m2\033[0m - Executar teste padrão (3 threads x 10 msgs)"
  echo -e " \033[96m3\033[0m - Executar teste custom (args: <threads> <msgs>)"
  echo -e " \033[96m4\033[0m - Limpar"
  echo -e " \033[96m0\033[0m - Sair"
  read -p "Escolha: " opt

  case "$opt" in
    1)
      make test
      clear
      ;;
    2)
      ./log_test
      exit 0
      ;;
    3)
      read -p "Threads: " t
      read -p "Msgs: " m
      ./log_test $t $m
      exit 0
      ;;
    4)
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
