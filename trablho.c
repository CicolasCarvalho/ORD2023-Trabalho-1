#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

//---------------------------------------------------------------
// struct para a implementação da LED.
typedef struct { // Nó de Espaço Disponível.
    char* id_removido;
    short tam_registro;
    struct NED* prox;
} NED;

typedef struct {
    char tipo;
    char arg[10];
} operacao;

// A função ler_op le o tipo de operação especificado no arquivo e o argumento
// passado logo após a operação:
// exemplo: b 23 | aqui ele retorna o tipo de operação "b" e o argumento
// referente à operação "23", ou seja, é uma busca ao identificador 23.
operacao ler_op(FILE *fd) {
    operacao op;

    op.tipo = fgetc(fd);
    if (fgetc(fd) != ' ') assert(false);

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
    if (argc == 3 && strcmp(argv[1], "-e") == 0) {

        printf("Modo de execucao de operacoes ativado ... nome do arquivo = %s\n", argv[2]);
        // chamada da funcao que executa o arquivo de operacoes
        // o nome do arquivo de operacoes estara armazenado na variavel argv[2]
        // executa_operacoes(argv[2])

        executa_op(argv[2]);

    } else if (argc == 2 && strcmp(argv[1], "-p") == 0) {

        printf("Modo de impressao da LED ativado ...\n");
        // chamada da funcao que imprime as informacoes da led
        // imprime_led();

    } else {

        fprintf(stderr, "Argumentos incorretos!\n");
        fprintf(stderr, "Modo de uso:\n");
        fprintf(stderr, "$ %s -e nome_arquivo\n", argv[0]);
        fprintf(stderr, "$ %s -p\n", argv[0]);
        exit(EXIT_FAILURE);

    }

    return 0;
}

// A função "executa_op" checa qual a operação passada no arquivo de comandos
// e executa as específicas funções de apoio.
void executa_op(char* path){
    FILE *fd;
    char buffer[150] = "\0";

    fd = fopen(path, "r");
    assert(fd != NULL);

        FILE* dados = fopen("dados.dat", "rb");
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
                    fclose(dados);
                    dados = fopen("dados.dat", "rb+");

                    int pos = remove_id(dados, op.arg);

                    if(pos < 0) {
                        printf("id não encontrado!\n");
                    } else {
                        fseek(dados, pos, SEEK_SET);
                        ler_campo(dados, buffer);
                        printf("Registro de id: %s removido.\n");
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

// A função "get_tam_registro" utiliza o descritor de arquivo passado e
// caso o offset passado na função seja negativo, ela retorna o tamanho
// do registro. Em caso contrário, ela retorna o tamanho do registro específicado
// no byte-offset.
short get_tam_registro(FILE* fd, int offset) {
    if (offset >= 0) {
        fseek(fd, offset, SEEK_SET);
    }

    short tamanho_registro;
    fread(&tamanho_registro, sizeof(short), 1, fd);

    return tamanho_registro;
}

// A função "fpeek" verifica o elemento à frente da posição do cursor movendo
// ele e, em seguida o retornando para o lugar anterior.
char fpeek(FILE* fd) {
    char c = fgetc(fd);
    ungetc(c, fd);

    return c;
}

// A função "ler_registro" lê o registro total eo armazena em uma string usada como buffer.
int ler_registro(FILE* fd, char* str) {
    checar_cabecalho(fd);

    short tamanho_registro = get_tam_registro(fd, -1);

    for (int i = 0; i < tamanho_registro; i++) {
        str[i] = fgetc(fd);
    }

    str[tamanho_registro] = '\0';
    return tamanho_registro;
}

// A função "ler campos" lê um único campo de um registro e o armazena em uma string
// auxiliar passada como parãmetro.
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

// A função "checar_cabeçalho" verifica se o cursor esta na área de 4 bytes do
// cabeçalho no início do arquivo e, caso esteja, move ela para a posição 
// de leitura do tamanho do registro. 
void checar_cabecalho(FILE* fd) {
    int pos = ftell(fd);
    if (pos < 0) assert(false);
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);
}

// A função "busca_id" retorna "verdadeiro" se o identificador passado no
// arquivo de operações está no arquivo "dados.dat" ou "falso" se não está.
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

    return false;
}

// A função "remove_id" remove um registro
// lógicamente e retorna qual id foi removido.
int remove_id(FILE* fd, char* id){
    int pos; 

    pos = busca_id(fd, id);
    if (pos == -1) return -1;

    fputc('*', fd);

    return ftell(fd) - 3;
}

// b 1
// b 100
// b 235