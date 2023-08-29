// Nícolas dos Santos Carvalho - 128660
// Hudson Henrique da Silva    - 128849
// Ciencia da Computacao - UEM
// 29/08/2023

// Esse programa foi desenvolvido para o trabalho 1
// de ORD ministrado pelo professor Paulo Roberto

// Consiste na realização de operações de insercao, remocao e busca
// em arquivos com registros de tamanhos variaveis, e manipulacao
// da LED no arquivo para otimização das operacoes.

// --------------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define LIMIAR_DE_INSERCAO 10

// --------------------------------------------------------------------------------------------------------------------------

typedef struct LED LED;

struct LED {
    int offset;
    short tam_registro;
    LED *prox;
};

LED *LED_criar(int offset, short tam){
    LED *ptr = malloc(sizeof(LED));

    ptr->offset = offset;
    ptr->tam_registro = tam;
    ptr->prox = NULL;

    return ptr;
}

// ela adiciona o elemento na fila em ordem decrescente de tamanho de registro
void LED_adicionar(LED *led, LED *novo) {
    if (led == NULL) {
        printf("tentou adicionar em fila vazia\n");
        assert(false);
    }

    if (led->prox == NULL || novo->tam_registro > led->prox->tam_registro) {
        novo->prox = led->prox;
        led->prox = novo;
        return;
    }

    LED_adicionar(led->prox, novo);
}

void LED_imprime(LED *led) {
    if (led == NULL) {
        printf("[offset: -1]");
        return;
    }

    if (led->offset == 0)
        printf("LED -> ");
    else
        printf("[offset: %i, tam: %i] -> ", led->offset, (int)led->tam_registro);
    
    LED_imprime(led->prox);
}

int LED_tam(LED *led) {
    if (led->prox == NULL)
        return 0;

    return LED_tam(led->prox) + 1;
}

// remove o primeiro elemento da fila, caso ele exista
// caso contrario, retorna NULL
LED *LED_remove(LED *led) {
    if (led == NULL) {
        printf("tentou remover em fila vazia\n");
        assert(false);
    }

    if (led->prox == NULL) return NULL;

    LED *removido = led->prox;
    led->prox = led->prox->prox;
    removido->prox = NULL;

    return removido;
}

void LED_free(LED *led) {
    if (led == NULL) return;

    LED_free(led->prox);
    free(led);
}

typedef struct {
    char tipo;
    char arg[200];
} operacao;

// le uma operacao do arquivo de operacoes
operacao ler_op(FILE *fd) {
    operacao op;

    op.tipo = fgetc(fd);
    if (fgetc(fd) != ' ') assert(false);

    fgets(op.arg, 200, fd);

    if (op.arg[strlen(op.arg) - 1] == '\n')
        op.arg[strlen(op.arg) - 1] = '\0';
    else
        op.arg[strlen(op.arg)] = '\0';

    return op;
}

// --------------------------------------------------------------------------------------------------------------------------

void executa_op(FILE *dados, char *path);
void op_busca(FILE *dados, char *str);
void op_inserir(FILE *dados, char *str, LED *led);
void op_remove(FILE *dados, char *str, LED *led);

int ler_registro(FILE *fd, char *str);
int ler_campo(FILE *fd, char *str);
short get_tam_registro(FILE *fd, int offset);
void checar_cabecalho(FILE *fd);
char fpeek(FILE *fd);

int busca_id(FILE *fd, char *id);
int insere_reg(FILE *fd, char *reg, LED *led);
int remove_id(FILE *fd, char *id, LED *led);

LED *led_montar(FILE *fd);
LED *led_ler_registro(FILE *fd, int *prox);
void led_escrever(FILE *fd, LED *led);
void led_atualizar_dados(FILE *fd, LED *led);

int main(int argc, char *argv[]) {
    FILE *dados = fopen("dados.dat", "rb+");
    if (dados == NULL) {
        printf("Erro: não foi possivel abrir aquivo de dados (dados.dat)");
        assert(false);
    }

    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        printf("Modo de execucao de operacoes ativado ... nome do arquivo = %s\n", argv[2]);
        executa_op(dados, argv[2]);
    } else if (argc == 2 && strcmp(argv[1], "-p") == 0) {
        printf("Modo de impressao da LED ativado ...\n");
        
        LED *led = led_montar(dados);
        LED_imprime(led);
        printf("\nTotal: %i espacos disponiveis\n", LED_tam(led));

        LED_free(led);
    } else {
        fprintf(stderr, "Argumentos incorretos!\n");
        fprintf(stderr, "Modo de uso:\n");
        fprintf(stderr, "$ %s -e nome_arquivo\n", argv[0]);
        fprintf(stderr, "$ %s -p\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fclose(dados);
    return 0;
}

// executa operacao a operacao do arquivo de operacoes
void executa_op(FILE *dados, char *path) {
    FILE *fd = fopen(path, "r");
    assert(fd != NULL);

    LED *led = led_montar(dados);

    while(fpeek(fd) != EOF) {
        operacao op = ler_op(fd);

        switch(op.tipo) {
            case 'b':
                op_busca(dados, op.arg);
                break;
            case 'i':
                op_inserir(dados, op.arg, led);
                break;
            case 'r': 
                op_remove(dados, op.arg, led);
                break;
            default:
                printf("argumento invalido! (%c)", op.tipo);
                assert(false);
        }
    }

    led_atualizar_dados(dados, led);
    
    LED_free(led);
    fclose(fd);
}

// realiza a busca pelo registro de chave str
// caso o registro seja encontrado, imprime o registro
void op_busca(FILE *dados, char *str) {
    char buffer[200] = "\0";
    printf("Busca pelo registro de chave \"%s\"\n", str);

    int pos = busca_id(dados, str);

    if (pos < 0) {
        printf("Erro: registro nao encontrado!\n\n");
    } else {
        fseek(dados, pos, SEEK_SET);
        ler_registro(dados, buffer);
        printf("%s (%i bytes)\n\n", buffer, (int)strlen(buffer));
    }
}

// realiza a insercao do registro str
// caso o registro ja exista, nao realiza a insercao
void op_inserir(FILE *dados, char *str, LED *led) {
    char buffer[200] = "\0";
    char id[200];
    strcpy(id, str);
    strtok(id, "|");
    printf("Insercao do registro de chave \"%s\" (%i bytes)\n", id, (int)strlen(str));
    if (busca_id(dados, id) >= 0) {
        printf("Nao foi inserido, pois ja existe um registro com a chave \"%s\"\n\n", id);
        return;
    }

    int pos = insere_reg(dados, str, led);

    if (pos < 0) {
        printf("Local: fim do arquivo\n\n");
    } else {
        fseek(dados, pos, SEEK_SET);
        ler_registro(dados, buffer);
        printf("\nLocal: offset = %i bytes (0x%x)\n\n", pos, pos);
    }
}

// realiza a remocao do registro de chave str
// caso o registro nao exista, nao realiza a remocao
void op_remove(FILE *dados, char *str, LED *led) {
    printf("Remocao do registro de chave \"%s\"\n", str);

    short tam = get_tam_registro(dados, busca_id(dados, str));
    int pos = remove_id(dados, str, led);

    if (pos < 0) {
        printf("Erro: Registro nao encontrado!\n\n");
    } else {
        printf("Registro removido! (%i bytes)\n", tam);
        printf("Local: offset = %i bytes (0x%x)\n\n", pos, pos);
    }
}

short get_tam_registro(FILE *fd, int offset) {
    if (offset >= 0) {
        fseek(fd, offset, SEEK_SET);
    }

    short tamanho_registro;
    fread(&tamanho_registro, sizeof(short), 1, fd);

    return tamanho_registro;
}

char fpeek(FILE *fd) {
    char c = fgetc(fd);
    ungetc(c, fd);

    return c;
}

// le caracter a caracter de um registro ate encontrar um *
// retorna o tamanho do registro lido
int ler_registro(FILE *fd, char *str) {
    checar_cabecalho(fd);

    short tamanho_registro = get_tam_registro(fd, -1);
    int i = 0;
    char c;

    while(i < tamanho_registro) {
        c = fgetc(fd);
        if (c == '*') break;
        str[i++] = c;
    }

    str[i] = '\0';
    return i;
}

// le caracter a caracter de um campo ate encontrar um |
// retorna o tamanho do campo lido
int ler_campo(FILE *fd, char *str) {
    checar_cabecalho(fd);
    int i = 0;
    char c;

    while ((c = fgetc(fd)) != '|' && c != '*') {
        str[i++] = c;
    }

    if (c == '*') str[0] = '\0';
    str[i] = '\0';
    return i + 1;
}

void checar_cabecalho(FILE *fd) {
    int pos = ftell(fd);
    if (pos < 0) assert(false);
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);
}

// retorna a posicao do registro de chave id
// caso nao encontre, retorna -1
int busca_id(FILE *fd, char *id) {
    char buffer[10] = "\0";
    int i = 0;
    short tam = 0;

    fseek(fd, 4, SEEK_SET);

    while (fpeek(fd) != EOF) {
        tam = get_tam_registro(fd, -1);
        i = ler_campo(fd, buffer);

        if (strcmp(buffer, id) == 0) {
            return ftell(fd) - i - 2;
        }

        fseek(fd, (int)tam - i, SEEK_CUR);
    }

    return -1;
}

// insere o registro reg no arquivo de dados
// caso nao seja possivel inserir o registro no espaco disponivel
// ele insere no fim do arquivo
int insere_reg(FILE *fd, char *reg, LED *led) {
    short tam_reg = strlen(reg);

    if (led->prox == NULL || led->prox->tam_registro < tam_reg) {
        fseek(fd, 0, SEEK_END);
        fwrite(&tam_reg, sizeof(short), 1, fd);
        fputs(reg, fd);
        return -1;
    }

    LED *removido = LED_remove(led);
    int offset = removido->offset;
    short tam = removido->tam_registro;

    int novo_offset = offset + (2 + tam_reg);
    short novo_tam = tam - (2 + tam_reg);

    printf("Tamanho do espaco reutilizado: %i bytes", tam);

    if (novo_tam <= LIMIAR_DE_INSERCAO) tam_reg = tam;

    fseek(fd, offset, SEEK_SET);
    fwrite(&tam_reg, sizeof(short), 1, fd);
    fputs(reg, fd);

    if (novo_tam > LIMIAR_DE_INSERCAO) {
        printf(" (Sobra de %i bytes)", novo_tam);

        LED *novo = LED_criar(novo_offset, novo_tam);
        LED_adicionar(led, novo);

        fwrite(&novo_tam, sizeof(short), 1, fd);
    }
    
    if(novo_tam >= 1) fputc('*', fd);

    free(removido);
    return offset;
}

// remove o registro de chave id do arquivo de dados
// caso nao seja possivel remover o registro, retorna -1
int remove_id(FILE *fd, char *id, LED *led) {
    int pos = busca_id(fd, id);
    short tam = get_tam_registro(fd, pos);

    if (pos <= -1) return -1;

    fseek(fd, pos + 2, SEEK_SET);
    fputc('*', fd);

    LED *novo = LED_criar(pos, tam);
    LED_adicionar(led, novo);

    return pos;
}

// monta a LED a partir do arquivo de dados
LED *led_montar(FILE *fd) {
    fseek(fd, 0, SEEK_SET);

    int prox = 0;
    LED *led = led_ler_registro(fd, &prox);

    while (prox > 0) {
        fseek(fd, prox, SEEK_SET);

        LED *aux = led_ler_registro(fd, &prox);
        LED_adicionar(led, aux);
    }

    return led;
}

// le um registro da LED a partir do arquivo de dados
// por referencia retorna o offset do proximo LED
// retorna o LED lido
LED *led_ler_registro(FILE *fd, int *prox) {
    int ofsset_atual = ftell(fd);
    short tam = 0;

    if (ftell(fd) >= 4) {
        tam = get_tam_registro(fd, -1);
        assert(fgetc(fd) == '*');
    }

    fread(prox, sizeof(int), 1, fd);
    LED *led = LED_criar(ofsset_atual, tam);

    return led;
}

// escreve o LED no arquivo de dados
void led_escrever(FILE *fd, LED *led) {
    if (ftell(fd) >= 4) {
        fwrite(&led->tam_registro, sizeof(short), 1, fd);
        fputc('*', fd);
    }

    int prox = led->prox == NULL ? -1: led->prox->offset;
    fwrite(&prox, sizeof(int), 1, fd);
}

// atualiza os dados da LED no arquivo de dados
// escrevendo os novos offsets, e os novos tamanhos de registro
void led_atualizar_dados(FILE *fd, LED *led) {
    LED *aux = led;

    do {
        fseek(fd, aux->offset, SEEK_SET);
        led_escrever(fd, aux);
        aux = aux->prox;
    } while(aux != NULL);
}