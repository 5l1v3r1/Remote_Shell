#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /** EXIT_SUCCESS; **/
#include <errno.h>      /** perror **/
#include <unistd.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close (s)

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;


#define ERROR(expression)   printf("ERROR : %s\nError Number : %d\nError Message : %s\n", expression, errno, strerror(errno));

void viderBuffer(char *buffer);
void nEraser(char *buffer);

int main(int argc, char *argv[])
{
    SOCKET sock;
    SOCKADDR_IN hostsin;
    socklen_t recsize = sizeof(hostsin);

    int ctrl;
    int msg_len = 0;
    int data_len = 0;

    char buffer[BUFSIZ] = "";
    char bufferCmd[BUFSIZ] = "";

    FILE *log_cmd_results = NULL;

    hostsin.sin_family  = AF_INET;
    hostsin.sin_port    = htons(1234);
    hostsin.sin_addr.s_addr = inet_addr("127.0.0.1"); // entrez ici l'adresse du server ou laisser l'ip local (192.168.1.25)
    memset(hostsin.sin_zero, 0, 8);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == SOCKET_ERROR)
    {
        ERROR("socket()");
        exit(-1);
    }

    ctrl = connect(sock, (SOCKADDR*)&hostsin, recsize);

    if(ctrl == SOCKET_ERROR)
    {
        ERROR("connect()");
        exit(-1);
    }

    /* Reception de la longueur du message d'acceuil */
    if(recv(sock, (char*)&msg_len, sizeof(msg_len), 0) == SOCKET_ERROR)
    {
        ERROR("recv() msg_len");
        exit(-1);
    }

    if(recv(sock, buffer, msg_len, 0) == SOCKET_ERROR)
    {
        ERROR("recv() msg");
        exit(-1);
    }

    printf("%s\n", buffer);

    viderBuffer(buffer);

    viderBuffer(bufferCmd);

    log_cmd_results = fopen("/var/log/cmd_results.log", "w");
    if(log_cmd_results == NULL)
        ERROR("fopen()");

    do
    {
        printf("\nEnter a command (quit for close the remote shell) :\n>");
        if(fgets(bufferCmd, 32, stdin) == NULL)
                ERROR("fgets() saisis cmd");

        nEraser(bufferCmd);

        /* Ecriture du nom de la commande dans le fichier log */
        if(fputs("Commande ", log_cmd_results) == EOF)
            ERROR("fputs() bufferCmd");

        if(fputs(bufferCmd, log_cmd_results) == EOF)
            ERROR("fputs() bufferCmd");

        data_len = strlen(bufferCmd);

        /* Envoie de la cmd */
        if(send(sock, bufferCmd, data_len, 0) == SOCKET_ERROR)
            ERROR("send() bufferCmd\n");

        if(strcmp(bufferCmd, "quit") == 0)
        {
            close(sock);
            printf("[+] Disonnected from the server.\n");
            return EXIT_SUCCESS;
        }

        /* Recpetion sortie de cmd */
        if(recv(sock, (char*)&data_len, sizeof(data_len), 0) == SOCKET_ERROR)
            ERROR("recv() results");

        if(recv(sock, buffer, BUFSIZ, 0) == SOCKET_ERROR)
            ERROR("recv() results");

        printf("%s ", buffer);

        /* Ecriture de la sortie de commande dans le fichier log */

        if(fputs(":\n", log_cmd_results) == EOF)
            ERROR("fputs() :\\n");

        if(fputs(buffer, log_cmd_results) == EOF)
            ERROR("fputs() buffer");

        if(fputs("\n\n", log_cmd_results) == EOF)
            ERROR("fputs() :\\n x 2");

    }while(strcmp(bufferCmd, "quit") != 0);

    close(sock);

    return EXIT_SUCCESS;
}

void viderBuffer(char *buffer)
{
    memset(buffer, 0, BUFSIZ);
}

void nEraser(char *chaine)
{
    size_t i = 0;

    for(i = 0; i < strlen(chaine); i++)
    {
        if(chaine[i] == '\n')
            chaine[i] = '\0';
    }
}



