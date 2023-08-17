#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

//---------------------------------------------------------------
typedef struct NED NED;

// struct para a implementaÃ§Ã£o da LED.
struct NED { // NÃ³ de EspaÃ§o DisponÃ­vel.
    int offset;
    short tam_registro;
    NED* prox;
};

NED* NED_criar(int offset, short tam){// funÃ§Ã£o nÃ£o testada
    NED* ptr = malloc(sizeof(NED));

    ptr->offset = offset;
    ptr->tam_registro = tam;
    ptr->prox = NULL;

    return ptr;
}

void NED_adicionar(NED *ned, NED* novo) {
    if (ned == NULL) {
        printf("tentou inserir em fila vazia\n");
        assert(false);
    }

    if (ned->prox == NULL) {
        ned->prox = novo;
        return;
    }

    NED_adicionar(ned->prox, novo);
}

// void cria_LED(NED* ptr, char* id, short tam) { // funÃ§Ã£o nÃ£o testada.
//     NED* temporario = NULL;
//     NED* percorre_LED;

//     if(ptr == NULL) {
//         ptr = NED_criar(id, tam);
//     } else {
//         percorre_LED = ptr;
//         while(percorre_LED->prox != NULL) {
//             percorre_LED = percorre_LED->prox;
//         }

//         temporario = NED_criar(id, tam);
//         percorre_LED->prox = temporario;
//     }
// }

typedef struct {
    char tipo;
    char arg[10];
} operacao;

// A funÃ§Ã£o ler_op le o tipo de operaÃ§Ã£o especificado no arquivo e o argumento
// passado logo apÃ³s a operaÃ§Ã£o:
// exemplo: b 23 | aqui ele retorna o tipo de operaÃ§Ã£o "b" e o argumento
// referente Ã  operaÃ§Ã£o "23", ou seja, Ã© uma busca ao identificador 23.
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

// A funÃ§Ã£o "executa_op" checa qual a operaÃ§Ã£o passada no arquivo de comandos
// e executa as especÃ­ficas funÃ§Ãµes de apoio.
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
                        printf("id nÃ£o encontrado!\n");
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

// A funÃ§Ã£o "get_tam_registro" utiliza o descritor de arquivo passado e
// caso o offset passado na funÃ§Ã£o seja negativo, ela retorna o tamanho
// do registro. Em caso contrÃ¡rio, ela retorna o tamanho do registro especÃ­ficado
// no byte-offset.
short get_tam_registro(FILE* fd, int offset) {
    if (offset >= 0) {
        fseek(fd, offset, SEEK_SET);
    }

    short tamanho_registro;
    fread(&tamanho_registro, sizeof(short), 1, fd);

    return tamanho_registro;
}

// A funÃ§Ã£o "fpeek" verifica o elemento Ã  frente da posiÃ§Ã£o do cursor movendo
// ele e, em seguida o retornando para o lugar anterior.
char fpeek(FILE* fd) {
    char c = fgetc(fd);
    ungetc(c, fd);

    return c;
}

// A funÃ§Ã£o "ler_registro" lÃª o registro total eo armazena em uma string usada como buffer.
int ler_registro(FILE* fd, char* str) {
    checar_cabecalho(fd);

    short tamanho_registro = get_tam_registro(fd, -1);

    for (int i = 0; i < tamanho_registro; i++) {
        str[i] = fgetc(fd);
    }

    str[tamanho_registro] = '\0';
    return tamanho_registro;
}

// A funÃ§Ã£o "ler campos" lÃª um Ãºnico campo de um registro e o armazena em uma string
// auxiliar passada como parÃ£metro.
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

// A funÃ§Ã£o "checar_cabeÃ§alho" verifica se o cursor esta na Ã¡rea de 4 bytes do
// cabeÃ§alho no inÃ­cio do arquivo e, caso esteja, move ela para a posiÃ§Ã£o
// de leitura do tamanho do registro.
void checar_cabecalho(FILE* fd) {
    int pos = ftell(fd);
    if (pos < 0) assert(false);
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);
}

// A funÃ§Ã£o "busca_id" retorna "verdadeiro" se o identificador passado no
// arquivo de operaÃ§Ãµes estÃ¡ no arquivo "dados.dat" ou "falso" se nÃ£o estÃ¡.
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

// A funÃ§Ã£o "remove_id" remove um registro
// lÃ³gicamente e retorna qual id foi removido.
int remove_id(FILE *fd, char *id) {
    int pos = busca_id(fd, id);

    if (pos <= -1) return -1;

    fseek(fd, pos+2, SEEK_SET);
    fputc('*', fd);

    return pos;
}

// b 1
// b 100
// b 235