#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

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

void LED_adicionar(LED *ned, LED* novo) {
    if (ned == NULL) {
        printf("tentou inserir em fila vazia\n");
        assert(false);
    }

    if (ned->prox == NULL) {
        ned->prox = novo;
        return;
    }

    LED_adicionar(ned->prox, novo);
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

void executa_op(char* path);
int ler_registro(FILE* fd, char* str);
int ler_campo(FILE* fd, char* str);

short get_tam_registro(FILE* fd, int offset);
void checar_cabecalho(FILE* fd);
char fpeek(FILE* fd);

int busca_id(FILE* fd, char* id);
int remove_id(FILE* fd, char* id);

int main(int argc, char *argv[]) {
    // if (argc == 3 && strcmp(argv[1], "-e") == 0) {

        printf("Modo de execucao de operacoes ativado ... nome do arquivo = %s\n", "ops.txt");
        // chamada da funcao que executa o arquivo de operacoes
        // o nome do arquivo de operacoes estara armazenado na variavel argv[2]
        // executa_operacoes(argv[2])

        executa_op("ops.txt");

    // } else if (argc == 2 && strcmp(argv[1], "-p") == 0) {

        // printf("Modo de impressao da LED ativado ...\n");
        // chamada da funcao que imprime as informacoes da led
        // imprime_led();

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

        while(!feof(fd)){
            operacao op = ler_op(fd);

            switch(op.tipo){
                case 'b':
                {
                    int pos = busca_id(dados, op.arg);

                    if (pos>= 0) {
                        fseek(dados, pos, SEEK_SET);
                        ler_registro(dados, buffer);
                        printf("Encontrou %s\n", buffer);
                    } else {
                        printf("Nao encontrou %s\n", op.arg);
                    }
                    break;
                }
                case 'i':
                    break;
                case 'r':
                {
                    int pos = remove_id(dados, op.arg);

                    if(pos < 0) {
                        printf("id nà£o encontrado!\n");
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

            // printf("%c\n", op.tipo);
            // printf("%s\n", op.arg);
        }
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

    while ((c = fgetc(fd)) != '|') {
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
int remove_id(FILE *fd, char *id) {
    int pos = busca_id(fd, id);

    if (pos <= -1) return -1;

    fseek(fd, pos+2, SEEK_SET);
    fputc('*', fd);

    return pos;
}