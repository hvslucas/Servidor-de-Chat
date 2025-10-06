# 	:computer::speech_balloon: Servidor de Chat Multiusuário (TCP)

- ### [:dart: Objetivo](#dart-objetivo-1)
- ### [:spiral_notepad: Comentários](#spiral_notepad-comentários-1)
- ### [:open_file_folder: Estrutura do Projeto](#open_file_folder-estrutura-do-projeto-1)
- ### [:gear: Como rodar](#gear-como-rodar-1)
- ### [:arrow_down: Baixar o projeto](https://github.com/hvslucas/chatmultiuser/archive/refs/heads/main.zip)

## Disciplina de Linguagem de Programação II (Programação Concorrente)

Esse foi um projeto desenvolvido por discentes do curso de *Engenharia da Computação da Universidade Federal da Paraíba*, curso este que pertence ao *[Centro de Informática](http://ci.ufpb.br/)*, localizado na *[Rua dos Escoteiros S/N - Mangabeira - João Pessoa - Paraíba - Brasil](https://g.co/kgs/xobLzCE)*. O projeto busca a implementação correspondente a uma versão modificada da Máquina Mic-1 na forma de uma máquina virtual em linguaguem de alto nível. O projeto foi avaliado por meio da verificação do funcionamento correto do projeto e a validação dos resultados esperados. 

### :speech_balloon: Autor:

-  :link:  *[Lucas Henrique Vieira da Silva](https://github.com/hvslucas)*

[![HJTUCODBWJKFJK4BUEG4ZA5XYU](https://github.com/user-attachments/assets/209319ab-fdd7-44b9-b73a-7badddcf1fd8)](#computerspeech_balloon-servidor-de-chat-multiusuário-tcp)

## :dart: Objetivo:

O objetivo deste projeto é desenvolver um sistema concorrente em rede completo (cliente/servidor TCP ou UDP) utilizando das técnicas de programação concorrente desenvolvidas ao decorrer da disciplina e recursos de comunicação entre processos locais ou em rede. Ao final, o projeto será avaliado a partir da saída e das boas práticas do código implementado.

### Descrição da etapa 1 - Biblioteca libtslog - Logging Thread-Safe

Implementação de uma biblioteca simples de **logging concorrente** em C (`libtslog`), utilizando **pthread mutex** para garantir exclusão mútua na escrita em arquivo.  

A biblioteca permite que múltiplas threads gravem mensagens de log de forma segura, evitando *race conditions*.  
Um programa de teste (`log_test.c`) simula várias threads concorrentes gerando mensagens.

### Descrição da etapa 2 - Servidor de Chat TCP + teste automático

Sistema completo de chat em rede com:
- **Servidor multithread**: aceita múltiplos clientes simultaneamente
- **Broadcast**: mensagens distribuídas para todos os usuários
- **Nicknames**: identificação personalizada dos usuários
- **Logging**: registro de todas as atividades com libtslog
- **Comunicação bidirecional**: threads separadas para envio e recebimento

### Etapa 3 - Sistema Completo com Otimizações 

uncionalidades Implementadas:
- **Arquitetura Produtor-Consumidor**: Thread worker dedicada para processamento de mensagens
- **Fila Thread-Safe**: Buffer circular com semáforos para controle de slots
- **Broadcast Assíncrono**: Mensagens processadas em background sem bloquear threads de cliente
- **Histórico de Mensagens**: Últimas 100 mensagens armazenadas e enviadas para novos clientes
- **Proteção Completa**: Mutex para estruturas compartilhadas (clientes, histórico)
- **Shutdown Graceful**: Finalização controlada de threads e liberação de recursos

**Técnicas de Concorrência Aplicadas:**
- `sem_t` (semáforos POSIX) para controle de filas
- `ThreadSafeQueue` como monitor para sincronização
- Variáveis atômicas para flags de controle

## :spiral_notepad: Comentários:

- Acabei não preenchendo nas etapas, pois fiz os relatórios presentes no diretório

## :open_file_folder: Estrutura do Projeto


```
.
├── bin/               # Binários compilados - Aparece após rodar o make
├── include/
│   ├── libtslog.h    # Header da biblioteca
│   └── thread_safe_queue.h # Header da fila thread-safe
├── src/
│   ├── libtslog.c           # Implementação da libtslog
│   ├── thread_safe_queue.c # Implementação da fila thread-safe
│   ├── servidor_chat.c      # Servidor multithread
│   └── cliente_chat.c       # Cliente do chat
├── tests/
│   ├── log_test.c        # Programa de teste (CLI com múltiplas threads)
│   └── script_teste.sh   # Script de teste automatizado
├── Makefile              # Automação da compilação
├── run.sh                # Script interativo de execução
└── README.md             # Este arquivo, documenta o projeto
```

O servidor utiliza uma arquitetura **Produtor-Consumidor** onde:
- **Threads dos clientes** atuam como produtores, colocando mensagens na fila
- **Thread worker** atua como consumidor, processando mensagens da fila e fazendo broadcast

### Arquitetura -- Etapa 1

[<img width="799" height="697" alt="Untitled design" src="https://github.com/user-attachments/assets/29ec098c-d3b8-4d30-9a77-c12403fceb58" />](#computerspeech_balloon-servidor-de-chat-multiusuário-tcp)


### Arquitetura -- Etapa 3

[<img width="799" height="697" alt="deepseek_mermaid_20251006_630429" src="https://github.com/user-attachments/assets/886b71e5-2920-452f-b1bf-7ac575f07812" />](#computerspeech_balloon-servidor-de-chat-multiusuário-tcp)


## :gear: Como rodar

[**Atenção:** Lembre de baixar o projeto e extraí-lo devidamente do `.zip`.](#computerspeech_balloon-servidor-de-chat-multiusuário-tcp)

### :package: Dependências

Para compilar e executar o projeto em **Linux**, é necessário ter instalados:

- **GCC** (GNU Compiler Collection)
- **pthread** (biblioteca de threads POSIX)
- Bibliotecas nativas da linguagem C

No Ubuntu/Debian, instale com:
```bash
sudo apt update
sudo apt install build-essential
```

### :arrow_forward: Compilação e Execução 

O projeto contém um script `.sh` que oferece algumas opções e, a partir da escolha, compila e executa automaticamente.

1. Dê permissão de execução ao script:

```bash
chmod +x run.sh
```

2. Executar o script:

```bash
./run.sh
```

3. O script irá fornecer as opções:
```
1 - Compilar (make)
2 - Executar teste padrão (3 threads x 10 msgs)
3 - Executar teste custom (args: <threads> <msgs>)
4 - Executar servidor
5 - Executar cliente
6 - Limpar
0 - Sair"
```

Algumas observações: 
- O script `run.sh` possui alguns recursos para ficar mais elegante ao usuário e talvez esses recursos fiquem modificados em outros terminais


### Teste automático

O teste automático realiza teste com 3 clientes e abre 2 terminais para o teste interativo.

Para rodar é analógo ao run.sh:

1. Dê permissão de execução ao script:

```bash
chmod +x ./tests/script_teste.sh
```

2. Executar o script:

```bash
./tests/script_teste.sh
```
