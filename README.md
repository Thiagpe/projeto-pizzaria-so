# Projeto Pizzaria Concorrente

## Equipe
- Daniel Raimundo
- Thiago Pereira

## Objetivo
Demonstrar concorrência e sincronização simulando uma pizzaria.

## Como Executar
1. Compile: `gcc pizzaria.c -o pizzaria -pthread`
2. Execute: `./pizzaria`

## Mecanismos Utilizados
- **Threads POSIX:** Garçons e Cozinheiros.
- **Processos (fork):** O entregador (Motoboy) é um processo separado.
- **Mutex:** Protege a fila de pedidos para não duplicar vendas.
- **Semáforos:** Controlam se há vagas na fila ou se há pedidos para cozinhar.
- **Pipe:** Envia o ID da pizza pronta da Thread (Cozinheiro) para o Processo (Entregador).
