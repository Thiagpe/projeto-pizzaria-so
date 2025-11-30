/*
 * PROJETO: Pizzaria Concorrente
 * OBJETIVO: Demonstrar uso de Threads, Processos, Mutex, Semáforos e Pipes.
 * SISTEMA: Linux (Ubuntu)
 * COMPILAÇÃO: gcc pizzaria.c -o pizzaria -pthread -lrt
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

// --- Configurações ---
#define NUM_GARCONS 2
#define NUM_COZINHEIROS 3
#define TAMANHO_FILA_PEDIDOS 5 // Capacidade máxima de pedidos aguardando (Buffer)

// --- Cores para o Terminal (Para ficar bonito na apresentação) ---
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define RESET   "\033[0m"

// --- Estruturas de Sincronização e Globais ---
int fila_pedidos[TAMANHO_FILA_PEDIDOS]; // Buffer compartilhado
int indice_entrada = 0;
int indice_saida = 0;

pthread_mutex_t mutex_fila; // Protege o acesso à fila (Exclusão Mútua)
sem_t sem_vagas;            // Conta quantas vagas vazias existem na fila
sem_t sem_pedidos;          // Conta quantos pedidos prontos existem para cozinhar

int pipe_entrega[2];        // IPC: Pipe para comunicação entre Cozinheiros (Threads) e Entregador (Processo)

// --- Função do Garçom (PRODUTOR - Threads) ---
void* rotina_garcom(void* arg) {
    long id = (long)arg;
    while (1) {
        int pedido_id = rand() % 100 + 1; // Gera um pedido aleatório
        sleep(rand() % 3 + 1); // Simula tempo para anotar pedido

        // Sincronização: Espera ter vaga na fila
        sem_wait(&sem_vagas);
        
        // Sincronização: Entra na Seção Crítica
        pthread_mutex_lock(&mutex_fila);
        
        fila_pedidos[indice_entrada] = pedido_id;
        indice_entrada = (indice_entrada + 1) % TAMANHO_FILA_PEDIDOS;
        
        printf(YELLOW "[Garçom %ld] Adicionou pedido #%d na fila.\n" RESET, id, pedido_id);
        
        pthread_mutex_unlock(&mutex_fila); // Sai da Seção Crítica
        
        // Sincronização: Avisa que tem um novo pedido disponível
        sem_post(&sem_pedidos);
    }
    return NULL;
}

// --- Função do Cozinheiro (CONSUMIDOR - Threads) ---
void* rotina_cozinheiro(void* arg) {
    long id = (long)arg;
    int pedido_pegado;

    while (1) {
        // Sincronização: Espera ter pedido para fazer
        sem_wait(&sem_pedidos);
        
        // Sincronização: Entra na Seção Crítica
        pthread_mutex_lock(&mutex_fila);
        
        pedido_pegado = fila_pedidos[indice_saida];
        indice_saida = (indice_saida + 1) % TAMANHO_FILA_PEDIDOS;
        
        printf(RED "  [Cozinheiro %ld] Preparando pedido #%d...\n" RESET, id, pedido_pegado);
        
        pthread_mutex_unlock(&mutex_fila); // Sai da Seção Crítica
        
        // Sincronização: Avisa que liberou uma vaga na fila (papelzinho saiu)
        sem_post(&sem_vagas);

        // Simula tempo de preparo
        sleep(rand() % 4 + 2);

        // IPC: Envia o pedido pronto para o Entregador via PIPE
        // Escreve no descritor de escrita do pipe (pipe_entrega[1])
        write(pipe_entrega[1], &pedido_pegado, sizeof(int));
        printf(RED "  [Cozinheiro %ld] Pedido #%d PRONTO! Enviado para entrega.\n" RESET, id, pedido_pegado);
    }
    return NULL;
}

// --- Função Principal ---
int main() {
    srand(time(NULL));

    // 1. Criar o Pipe (IPC)
    if (pipe(pipe_entrega) == -1) {
        perror("Erro ao criar pipe");
        return 1;
    }

    // 2. Inicializar Sincronização
    pthread_mutex_init(&mutex_fila, NULL);
    sem_init(&sem_vagas, 0, TAMANHO_FILA_PEDIDOS); // Começa cheio de vagas
    sem_init(&sem_pedidos, 0, 0);                  // Começa com 0 pedidos

    printf(BLUE "=== INICIANDO SISTEMA DA PIZZARIA ===\n" RESET);
    printf("Processo Pai (PID: %d) gerenciando Cozinha.\n", getpid());

    // 3. Criar Processo Filho (Entregador) com fork()
    pid_t pid_entregador = fork();

    if (pid_entregador < 0) {
        perror("Erro no fork");
        return 1;
    }

    if (pid_entregador == 0) {
        // --- CÓDIGO DO PROCESSO FILHO (ENTREGADOR) ---
        close(pipe_entrega[1]); // Fecha a ponta de escrita, ele só lê
        int pedido_recebido;
        
        printf(GREEN ">>> Processo Entregador (PID: %d) pronto para entregas.\n" RESET, getpid());

        // Loop infinito lendo do pipe
        while (read(pipe_entrega[0], &pedido_recebido, sizeof(int)) > 0) {
            printf(GREEN ">>> [Motoboy] Pegou o pedido #%d e saiu para entrega!\n" RESET, pedido_recebido);
            sleep(1); // Simula tempo de entrega
        }
        
        close(pipe_entrega[0]);
        exit(0); // Fim do processo filho
    } 
    else {
        // --- CÓDIGO DO PROCESSO PAI (GERENTE/COZINHA) ---
        close(pipe_entrega[0]); // Fecha a ponta de leitura, a cozinha só escreve

        pthread_t garcons[NUM_GARCONS];
        pthread_t cozinheiros[NUM_COZINHEIROS];

        // Cria Threads Garçons
        for (long i = 0; i < NUM_GARCONS; i++) {
            pthread_create(&garcons[i], NULL, rotina_garcom, (void*)(i+1));
        }

        // Cria Threads Cozinheiros
        for (long i = 0; i < NUM_COZINHEIROS; i++) {
            pthread_create(&cozinheiros[i], NULL, rotina_cozinheiro, (void*)(i+1));
        }

        // Aguarda as threads (na prática, como é um loop infinito, fica aqui pra sempre)
        for (int i = 0; i < NUM_GARCONS; i++) pthread_join(garcons[i], NULL);
        for (int i = 0; i < NUM_COZINHEIROS; i++) pthread_join(cozinheiros[i], NULL);

        // Limpeza (Só chega aqui se o loop for quebrado, ex: CTRL+C manual)
        pthread_mutex_destroy(&mutex_fila);
        sem_destroy(&sem_vagas);
        sem_destroy(&sem_pedidos);
        close(pipe_entrega[1]);
        wait(NULL); // Espera o filho terminar
    }

    return 0;
}
