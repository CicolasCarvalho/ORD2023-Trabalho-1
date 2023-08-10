#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

//---------------------------------------------------------------

typedef struct {
    char tipo;
    char arg[150];
} operacao;

operacao ler_op(FILE *fd) {
    operacao op;

    op.tipo = fgetc(fd);

    fseek(fd, 1, SEEK_CUR);
    fgets(op.arg, 150, fd);

    if (op.arg[strlen(op.arg) - 1] == '\n')
        op.arg[strlen(op.arg) - 1] = '\0';

    return op;
}

//---------------------------------------------------------------

void executa_op(char* path);
int ler_registro(FILE* fd, char* str);
int get_tam_registro(FILE* fd, int offset);

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

void executa_op(char* path){
    FILE* fd;
    char buffer[150];

    fd = fopen(path, "r");
        assert(fd != NULL);

        FILE* dados = fopen("dados.dat", "rb");
        assert(dados != NULL);

        while(!feof(fd)){
            operacao op = ler_op(fd);

            switch(op.tipo){
                case 'b':
                    int i = ler_registro(dados, buffer);
                    
                    printf("lido %i bytes: %s\n", i, buffer);
                    break;
                case 'i':
                    break;
                case 'r':
                    break;
                default:
                    assert(false);
            }
            
            // printf("%c\n", op.tipo);
            // printf("%s\n", op.arg);
        }
        fclose(dados);
    fclose(fd);

}

int get_tam_registro(FILE* fd, int offset) {
    fseek(fd, offset, SEEK_SET);

    short tamanho_registro;
    fread(&tamanho_registro, sizeof(short), 1, fd);

    return tamanho_registro;
}

int ler_registro(FILE* fd, char* str) {
    int pos = ftell(fd);
    if (pos < 0) assert(false); 
    if (pos < 4)
        fseek(fd, 4, SEEK_SET);

    int tamanho_registro = get_tam_registro(fd, ftell(fd));

    for (int i = 0; i < tamanho_registro; i++) {
        str[i] = fgetc(fd);
    }

    str[tamanho_registro] = '\0';
    return tamanho_registro;
}