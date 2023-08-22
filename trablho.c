#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

// tnc (toma no cu) -> (tudo nos conformes)
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
    ptr->prox = NULL;

    return ptr;
}

void LED_adicionar(LED *led, LED *novo) {
    if (led == NULL) {
        printf("tentou inserir em fila vazia\n");
        assert(false);
    }

    // if (led->tam_registro < novo->tam_registro) {
    //     LED aux = *led;
    //     novo->prox = &aux;
    //     led = novo;
    //     return;

    //     // novo->prox = led->prox;
    //     // led->prox = novo;
    //     // return;
    // }

    if (led->prox == NULL) {
        led->prox = novo;
        return;
    }

    LED_adicionar(led->prox, novo);
}

void LED_imprime(LED *led) {
    if (led == NULL)
        return;

    printf("[offset: %i", led->offset, (int)led->tam_registro);

    if (led->tam_registro <= 0) {
        printf("] -> ");
    } else {
        printf(", tam: %i] -> ", (int)led->tam_registro);
    }

    LED_imprime(led->prox);
}

typedef struct {
    char tipo;
    char arg[10];
} operacao;

// Lê uma operação do arquivo de operações.
// "b 123" -> {tipo = 'b', arg = "123"}
operacao ler_op(FILE *fd) {
    operacao op;

    op.tipo = fgetc(fd);
    char c = fgetc(fd);
    if (c != ' ') assert(false);

    fgets(op.arg, 10, fd);

    if (op.arg[strlen(op.arg) - 1] == '\n')
        op.arg[strlen(op.arg) - 1] = '\0';
    else
        op.arg[strlen(op.arg)] = '\0';

    // printf("%c, %s", op.tipo, op.arg);

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
int remove_id(FILE  *fd, char  *id, LED  *led);

LED *led_montar(FILE *fd);
LED *led_ler_registro(FILE *fd, int *prox);
void led_escrever(FILE *fd, LED *led);

int main(int argc, char *argv[]) {
    // if (argc == 3 && strcmp(argv[1], "-e") == 0) {

        // printf("Modo de execucao de operacoes ativado ... nome do arquivo = %s\n", argv[2]);
        // chamada da funcao que executa o arquivo de operacoes
        // o nome do arquivo de operacoes estara armazenado na variavel argv[2]
        // executa_operacoes(argv[2])

        // executa_op(argv[2]);
        executa_op("ops.txt");

    // } else if (argc == 2 && strcmp(argv[1], "-p") == 0) {

        // FILE* dados = fopen("dados.dat", "rb+");
        // assert(dados != NULL);
        // // printf("Modo de impressao da LED ativado ...\n");
        // // chamada da funcao que imprime as informacoes da led
        // LED_imprime(led_montar(dados));

        // fclose(dados);
    // } else {

    //     fprintf(stderr, "Argumentos incorretos!\n");
    //     fprintf(stderr, "Modo de uso:\n");
    //     fprintf(stderr, "$ %s -e nome_arquivo\n", argv[0]);
    //     fprintf(stderr, "$ %s -p\n", argv[0]);
    //     exit(EXIT_FAILURE);

    // }

    return 0;
}

// Executa as operações do arquivo de operações.
void executa_op(char* path){
    FILE *fd;
    char buffer[150] = "\0";

    fd = fopen(path, "r");
    assert(fd != NULL);

    FILE* dados = fopen("dados.dat", "rb+");
    assert(dados != NULL);

    LED *led = led_montar(dados);

    while(!feof(fd)) {
        operacao op = ler_op(fd);

        switch(op.tipo) {
            case 'b': {
                int pos = busca_id(dados, op.arg);

                if (pos < 0) {
                    printf("Nao encontrou %s\n", op.arg);
                } else {
                    fseek(dados, pos, SEEK_SET);
                    ler_registro(dados, buffer);
                    printf("Encontrou %s\n", buffer);
                }
                break;
            }
            case 'i':
                break;
            case 'r': {
                int pos = remove_id(dados, op.arg, led);

                if (pos < 0) {
                    printf("id nao encontrado!\n");
                } else {
                    fseek(dados, pos, SEEK_SET);
                    ler_registro(dados, buffer);
                    printf("Registro de id: %s removido.\n", buffer);
                }
                break;
            }
            default:
                assert(false);
        }
    }

    LED_imprime(led);

    fclose(dados);
    fclose(fd);
}

// o tamanho do registro na posição do cursor.
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

// A função "ler registro" lê um registro inteiro do arquivo de dados e o armazena
int ler_registro(FILE* fd, char* str) {
    checar_cabecalho(fd);

    short tamanho_registro = get_tam_registro(fd, -1);

    for (int i = 0; i < tamanho_registro; i++) {
        str[i] = fgetc(fd);
    }

    str[tamanho_registro] = '\0';
    return tamanho_registro;
}

// A função "ler_campo" lê um campo do arquivo de dados e o armazena
int ler_campo(FILE* fd, char* str) {
    checar_cabecalho(fd);

    int i = 0;
    char c;

    while ((c = fgetc(fd)) != '|' && c != '*') {
        str[i++] = c;
    }
    str[++i] = '\0';

    return i;
}

// A função "checar_cabecalho" checa se o ponteiro do arquivo está no cabeçalho
// e o corrige se não estiver.
void checar_cabecalho(FILE* fd) {
    int pos = ftell(fd);
    if (pos < 0) assert(false);
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);
}

// A função "busca_id" busca um registro pelo id e retorna a posição do registro
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

// A função "remove_id" remove um registro pelo id
// e retorna a posição do registro
int remove_id(FILE *fd, char *id, LED *led) {
    int pos = busca_id(fd, id);
    short tam = get_tam_registro(fd, pos);

    if (pos <= -1) return -1;

    fseek(fd, pos + 2, SEEK_SET);
    fputc('*', fd);

    fwrite(&led->offset, sizeof(int), 1, fd);
    LED_adicionar(led, LED_criar(pos, tam));

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
    int old_offset = ftell(fd);
    fseek(fd, led->offset < 0 ? 0 : led->offset, SEEK_SET);

    if (ftell(fd) >= 4) {
        fwrite(&led->tam_registro, sizeof(short), 1, fd);
        fputc('*', fd);
    }

    fwrite(&led->prox->offset, sizeof(int), 1, fd);
    fseek(fd, old_offset, SEEK_SET);
}