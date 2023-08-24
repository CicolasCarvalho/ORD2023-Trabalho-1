#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define LIMIAR_DE_INSERCAO 20

//---------------------------------------------------------------

typedef struct LED LED;

struct LED {
    int offset;
    short tam_registro;
    LED* prox;
};

LED* LED_criar(int offset, short tam){
    LED* ptr = malloc(sizeof(LED));

    ptr->offset = offset;
    ptr->tam_registro = tam;
    ptr->prox = ptr;

    return ptr;
}

void LED_adicionar(LED *led, LED *novo) {
    if (led == NULL) {
        printf("tentou inserir em fila vazia\n");
        assert(false);
    }

    if (novo->tam_registro > led->prox->tam_registro) {
        novo->prox = led->prox;
        led->prox = novo;
        return;
    }

    LED_adicionar(led->prox, novo);
}

void LED_imprime(LED *led) {
    printf("[offset: %i", led->offset, (int)led->tam_registro);

    if (led->tam_registro > 0) {
        printf(", tam: %i] -> ", (int)led->tam_registro);
    } else {
        printf("]");
    }

    if (led->offset < 0)
        return;

    LED_imprime(led->prox);
}

int LED_tam(LED *led) {
    if (led->offset <= -1 && led->prox->offset <= -1) { // indica lista vazia por causa da cabe�a
        return 0;
    }

    if (led->offset > -1 && led->prox->offset <= -1) { // pula a cabe�a
        return 0;
    }

    return LED_tam(led->prox) + 1;
}

typedef struct {
    char tipo;
    char arg[150];
} operacao;

operacao ler_op(FILE *fd) {
    operacao op;

    op.tipo = fgetc(fd);
    if (fgetc(fd) != ' ') assert(false);

    fgets(op.arg, 150, fd);

    if (op.arg[strlen(op.arg) - 1] == '\n')
        op.arg[strlen(op.arg) - 1] = '\0';
    else
        op.arg[strlen(op.arg)] = '\0';

    return op;
}

//---------------------------------------------------------------

void executa_op(char *path);
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
    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        printf("Modo de execucao de operacoes ativado ... nome do arquivo = %s\n", argv[2]);
        executa_op(argv[2]);
        // executa_op("ops.txt");
    } else if (argc == 2 && strcmp(argv[1], "-p") == 0) {
        FILE* dados = fopen("dados.dat", "rb+");
        assert(dados != NULL);

        printf("Modo de impressao da LED ativado ...\n");
        printf("LED -> ");
        LED *led = led_montar(dados);
        int tam = LED_tam(led);
        led = led->prox;
        LED_imprime(led);
        printf("\nTotal: %i espacos disponiveis", tam);

        fclose(dados);
    } else {
        fprintf(stderr, "Argumentos incorretos!\n");
        fprintf(stderr, "Modo de uso:\n");
        fprintf(stderr, "$ %s -e nome_arquivo\n", argv[0]);
        fprintf(stderr, "$ %s -p\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}

void executa_op(char* path){
    FILE *fd;
    char buffer[150] = "\0";

    fd = fopen(path, "r");
    assert(fd != NULL);

    FILE* dados = fopen("dados.dat", "rb+");
    assert(dados != NULL);

    LED *led = led_montar(dados);

    while(fpeek(fd) != EOF) {
        operacao op = ler_op(fd);

        switch(op.tipo) {
            case 'b': {
                printf("Busca pelo registro de chave \"%s\"\n", op.arg);

                int pos = busca_id(dados, op.arg);

                if (pos < 0) {
                    printf("Erro: registro nao encontrado!\n\n");
                } else {
                    fseek(dados, pos, SEEK_SET);
                    ler_registro(dados, buffer);
                    printf("%s (%lli bytes)\n\n", buffer, strlen(buffer));
                }
                break;
            }
            case 'i': {
                char id[150];
                strcpy(id, op.arg);
                strtok(id, "|");
                printf("Insercao do registro de chave \"%s\" (%lli bytes)\n", id, strlen(op.arg));

                int pos = insere_reg(dados, op.arg, led);

                if (pos < 0) {
                    printf("Local: fim do arquivo\n\n");
                } else {
                    fseek(dados, pos, SEEK_SET);
                    ler_registro(dados, buffer);
                    printf("\nLocal: offset = %i bytes (0x%x)\n\n", pos, pos);
                }
                break;
            }
            case 'r': {
                printf("Remocao do registro de chave \"%s\"\n", op.arg);

                short tam = get_tam_registro(dados, busca_id(dados, op.arg));
                int pos = remove_id(dados, op.arg, led);

                if (pos < 0) {
                    printf("Erro: Registro nao encontrado!\n\n");
                } else {
                    printf("Registro removido! (%i bytes)\n", tam);
                    printf("Local: offset = %i bytes (0x%x)\n\n", pos, pos);
                }
                break;
            }
            default:
                assert(false);
        }
    }

    led_atualizar_dados(dados, led);

    fclose(dados);
    fclose(fd);
}

short get_tam_registro(FILE* fd, int offset) {
    if (offset >= 0) {
        fseek(fd, offset, SEEK_SET);
    }

    short tamanho_registro;
    fread(&tamanho_registro, sizeof(short), 1, fd);

    return tamanho_registro;
}

char fpeek(FILE* fd) {
    char c = fgetc(fd);
    ungetc(c, fd);

    return c;
}

int ler_registro(FILE* fd, char* str) {
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

int ler_campo(FILE* fd, char* str) {
    checar_cabecalho(fd);

    int i = 0;
    char c;

    while ((c = fgetc(fd)) != '|' && c != '*') {
        str[i++] = c;
    }

    if (c == '*') str[0] = '\0';

    str[++i] = '\0';

    return i;
}

void checar_cabecalho(FILE* fd) {
    int pos = ftell(fd);
    if (pos < 0) assert(false);
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);
}

int busca_id(FILE* fd, char* id) {
    char buffer[10] = "\0";
    short i = 0;
    int tam = 0;

    fseek(fd, 4, SEEK_SET);

    while (fpeek(fd) != EOF) {
        tam = get_tam_registro(fd, -1);
        i = ler_campo(fd, buffer);

        if (strcmp(buffer, id) == 0) {
            return ftell(fd) - i - 2;
        }

        fseek(fd, tam-i, SEEK_CUR);
    }

    return -1;
}

int insere_reg(FILE *fd, char *reg, LED *led) {
    short tam_reg = strlen(reg);

    if (LED_tam(led) <= 0 || led->prox->tam_registro < (tam_reg + 2)) {
        fseek(fd, 0, SEEK_END);
        fwrite(&tam_reg, sizeof(short), 1, fd);
        fputs(reg, fd);
        return -1;
    }

    LED *removido = led->prox;
    led->prox = led->prox->prox;
    removido->prox = NULL;

    int offset = removido->offset;

    int novo_offset = offset + (2 + tam_reg);
    int novo_tam = removido->tam_registro - (2 + tam_reg);

    printf("Tamanho do espaco reutilizado: %i bytes", removido->tam_registro);

    if (novo_tam <= LIMIAR_DE_INSERCAO) {
        tam_reg += (novo_tam + 2);
    }

    fseek(fd, offset, SEEK_SET);
    fwrite(&tam_reg, sizeof(short), 1, fd);
    fputs(reg, fd);

    if (novo_tam > LIMIAR_DE_INSERCAO) {
        printf(" (Sobra de %i bytes)", novo_tam);

        LED *novo = LED_criar(novo_offset, novo_tam);
        LED_adicionar(led, novo);
    }

    free(removido);
    return offset;
}

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

LED *led_montar(FILE *fd) {
    fseek(fd, 0, SEEK_SET);

    int prox;
    LED *led = led_ler_registro(fd, &prox);
    led->offset = -1;

    while (prox >= 0) {
        fseek(fd, prox, SEEK_SET);

        LED *aux = led_ler_registro(fd, &prox);

        LED_adicionar(led, aux);
    }

    return led;
}

LED *led_ler_registro(FILE *fd, int *prox) {
    int ofsset_atual = ftell(fd);
    short tam = 0;

    if (ftell(fd) >= 4) {
        tam = get_tam_registro(fd, -1);
        assert(fgetc(fd) == '*');
    }

    fread(prox, sizeof(prox), 1, fd);

    LED *led = LED_criar(ofsset_atual, tam);

    return led;
}

void led_escrever(FILE *fd, LED *led) {
    if (ftell(fd) >= 4) {
        fwrite(&led->tam_registro, sizeof(short), 1, fd);
        fputc('*', fd);
    }

    fwrite(&led->prox->offset, sizeof(int), 1, fd);
}

void led_atualizar_dados(FILE *fd, LED *led) {
    LED *aux = led;

    do {
        fseek(fd, aux->offset < 0 ? 0 : aux->offset, SEEK_SET);
        led_escrever(fd, aux);
        aux = aux->prox;
    } while(aux->offset >= 4);
}