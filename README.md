# Trabalho 1 da disciplina de Organização e Recuperação de Dados

### Especificação
O arquivo dados.dat possui informações sobre jogos. Os dados dos jogos estão armazenados em registros de tamanho variável, em formato similar ao utilizado nas aulas práticas. O arquivo é iniciado por um cabeçalho que armazena um número inteiro de 4 bytes (int). Na sequência, estão armazenados os registros. Cada registro inicia com um campo que armazena o seu tamanho em bytes como um número inteiro de 2 bytes (short). O restante dos campos é do tipo texto e estão separados pelo caractere ‘|’.

### Busca, Inserção e Remoção
Dado o arquivo dados.dat, o seu programa deverá oferecer as seguintes funcionalidades principais:

- Busca de um jogo pelo IDENTIFICADOR;
- Inserção de um novo jogo;
- Remoção de um jogo.

As operações a serem realizadas em determinada execução serão especificadas em um arquivo de operações, o qual será passado ao programa como um parâmetro.

### Formato do Arquivo de Operações
Um arquivo inicial de operações foi disponibilizado no Classroom para que você possa utilizá-lo como exemplo. O arquivo de operações deve possuir uma operação por linha, codificada com o identificador da operação (b = busca, i = inserção ou r = remoção) e respectivos argumentos.

### Gerenciamento de Espaços Disponíveis
As alterações que venham a ocorrer no arquivo dados.dat deverão ser persistentes. Dessa forma, sempre que o programa for iniciado para a execução de operações, ele deverá abrir esse arquivo, se ele existir, para leitura e escrita.

A remoção de registros será lógica e o espaço disponível resultante da remoção deverá ser inserido na Lista de Espaços Disponíveis (LED). A LED deverá ser mantida no próprio arquivo e os ponteiros da LED devem ser gravados como números inteiros de 4 bytes (int). O seu programa deverá implementar todos os mecanismos necessários para o gerenciamento da LED e reutilização dos espaços disponíveis utilizando a estratégia pior ajuste (worst-fit).