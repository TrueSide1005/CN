#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wait.h>
#include <sys/sysmacros.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <netinet/in.h>

#define PORT 2024

extern int errno;
int port;
char msg[100];

int main(int argc, char *argv[])
{
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare
    char com[100];             // mesajul trimis
    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }
    /* stabilim portul */
    port = atoi(argv[2]);
    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]); //adresa IP a serverului
    server.sin_port = htons(port);               //port de conectare
    //conectare la server
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Connect error.\n");
        return errno;
    }
    int login = 0;
    while (1)
    {
        bzero(com, 100);
        if (login == 0)
            printf("\n\033[01;33m Available commands: \033[0m \033[1;34mlogin / exit / Join: \033[0m");
        else
            printf("\n\033[01;33m Available commands: \033[0m \033[1;34m exit / NewSong / ShowTop / ShowGenreTop / Vote / ModifyUserPermission / DeleteSong / Info / AddComment: \033[0m");
        fflush(stdout);
        //citesc comanda si o trimit la server
        scanf("%s", com);
        if (write(sd, com, 100) <= 0)
        {
            perror("[client]Write error - command. \n");
            return errno;
        }
        if ((strcmp(com, "login") == 0) && (login == 0)) //LOGIN
        {
            //citesc si trimit la server username-ul si parola
            printf("\033[1;34m Username: \033[0m");
            char usr[100], pas[100];
            fflush(stdout);
            bzero(usr, 100);
            scanf("%s", usr);
            if (write(sd, usr, 100) <= 0)
            {
                perror("[client]Write error - login.\n");
                return errno;
            }
            printf("\033[1;34m Password: \033[0m");
            fflush(stdout);
            bzero(pas, 100);
            scanf("%s", pas);
            if (write(sd, pas, 100) <= 0)
            {
                perror("[client]Write error - login.\n");
                return errno;
            }
            //primesc mesajul de la server care imi arata
            //daca utilizatorul exista sau nu in baza de date,
            //daca exista, acesta tocmai s-a conectat
            char log[1];
            bzero(log, 1);
            if (read(sd, log, 1) < 0)
            {
                perror("[client]Read error - login. \n");
                return errno;
            }
            if (log[0] == '1')
            {
                login++;
                printf("\n\033[01;33m Welcome!  \n\033[0m");
            }
            else
            {
                printf("\n\033[1;31m Incorrect username or password...  \n\033[0m");
                continue;
            }
        }
        else if ((strcmp(com, "login") == 0) && (login == 1)) //comanda e login, dar utilizatorul e deja conectat
        {
            printf("\033[1;31m You are already logged in... \n\033[0m");
            continue;
        }
        else if ((strcmp(com, "Join") == 0) && (login == 0)) //JOIN
        {
            char nume[100], parola[100];
            //citesc noul nue de utilizator si il trimit serverului pentru verificare
            printf("\033[1;34m New username: \033[0m");
            fflush(stdout);
            bzero(nume, 100);
            scanf(" %[^\t\n]s", nume);
            if (write(sd, nume, 100) <= 0)
            {
                perror("[client]Write error - Join - nume.\n");
                return errno;
            }
            char cont[1];
            bzero(cont, 1);
            if (read(sd, cont, 1) < 0)
            {
                perror("[client]Read error - join. \n");
                return errno;
            }
            if (cont[0] == '0')            //username-ul dat nu este deja in baza de date
            {
                printf("\033[01;33m Username accepted!  \n\033[0m");
                printf("\033[1;34m New password: \033[0m"); //citesc parola
                fflush(stdout);
                bzero(parola, 100);
                scanf(" %[^\t\n]s", parola);
                if (write(sd, parola, 100) <= 0)
                {
                    perror("[client]Write error - Join - parola.\n");
                    return errno;
                }
                char b[10];
                bzero(b, 10);
                if (read(sd, b, 10) < 0)
                {
                    perror("[client]Read error - JOIN. \n");
                    return errno;
                }
                if (b[0] == '1')
                    fprintf(stdout, "\n\033[01;33m User created successfully \n\033[0m");
                else
                {
                    fprintf(stdout, "\n\033[1;31m User couldn't be created \n\033[0m");
                    continue;
                }
            }             //username-ul dat este deja in baza de date
            else
            {
                printf("\033[1;31m Username already in use... \n\033[0m");
                continue;
            }
        }
        else if ((strcmp(com, "NewSong") == 0) && (login == 1)) //NEWSONG
        {
            //colectez datele despre melodia care va fi adaugata in tabela music si le trimit la server
            char title[50], artist[50], link[100], desc[300], gen[40];
            printf("\033[1;34m Give the title of the song: \033[0m");
            fflush(stdout);
            bzero(title, 50);
            scanf(" %[^\t\n]s", title);
            if (write(sd, title, 50) <= 0)
            {
                perror("[client]Write error - NewSong - title.\n");
                return errno;
            }
            printf("\033[1;34m Give the singer: \033[0m");
            fflush(stdout);
            bzero(artist, 50);
            scanf(" %[^\t\n]s", artist);
            if (write(sd, artist, 50) <= 0)
            {
                perror("[client]Write error - NewSong - artist.\n");
                return errno;
            }
            printf("\033[1;34m Give a link for the song: \033[0m");
            fflush(stdout);
            bzero(link, 100);
            scanf(" %[^\t\n]s", link);
            if (write(sd, link, 100) <= 0)
            {
                perror("[client]Write error - NewSong - link.\n");
                return errno;
            }
            fflush(stdout);
            printf("\033[1;34m Give a short description for the song: \033[0m");
            fflush(stdout);
            bzero(desc, 300);
            scanf(" %[^\t\n]s", desc);
            if (write(sd, desc, 300) <= 0)
            {
                perror("[client]Write error - NewSong - desc.\n");
                return errno;
            }
            printf("\033[1;34m Give the genre of the song (as many as it has): \033[0m");
            fflush(stdout);
            bzero(gen, 40);
            scanf(" %[^\t\n]s", gen);
            if (write(sd, gen, 40) <= 0)
            {
                perror("[client]Write error - NewSong - gen.\n");
                return errno;
            }
            char exista[10];
            bzero(exista, 10);
            if (read(sd, exista, 10) < 0)
            {
                perror("[client]Read error - NewSong. \n");
                return errno;
            }
            if (strcmp(exista, "NULL") == 0) //piesa nu exista deja in top
            {
                //verific mesajul serverului care spune daca datele au fost adaugate
                char b[10];
                bzero(b, 10);
                if (read(sd, b, 10) < 0)
                {
                    perror("[client]Read error - NewSong. \n");
                    return errno;
                }
                if (b[0] == '1')
                    fprintf(stdout, "\n\033[01;33m Records created successfully \n\033[0m");
                else
                {
                    fprintf(stdout, "\n\033[1;31m Records couldn't be created \n\033[0m");
                    continue;
                }
            }
            else
                printf("\n\033[1;31m The song does already exist! \n\033[0m");
        }
        else if ((strcmp(com, "ShowGenreTop") == 0) && (login == 1)) //SHOWGENRETOP
        {
            char gen[10];
            printf("\033[1;34m Give the genre: \033[0m"); //citesc genul dorit
            fflush(stdout);
            bzero(gen, 10);
            scanf(" %[^\t\n]s", gen);
            if (write(sd, gen, 10) <= 0)
            {
                perror("[client]Write error - ShowGenreTop - gen.\n");
                return errno;
            }
            printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
            fflush(stdout);
            //nr va memora cate melodii din acel gen exista
            char nr[10];
            if (read(sd, nr, 10) < 0)
            {
                perror("[client]Read error - ShowGenreTop. \n");
                return errno;
            }
            //afisez datele despre melodii, transmise linie cu linie, m melodii
            int m = atoi(nr);
            for (int j = 0; j < m; j++)
            {
                for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela users care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - ShowGenreTop. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                printf("\n");
            }
        }
        else if ((strcmp(com, "ShowTop") == 0) && (login == 1)) //SHOWTOP
        {
            printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
            char nr[10]; //va retine cate melodii se alfa in top
            if (read(sd, nr, 10) < 0)
            {
                perror("[client]Read error - ShowTop. \n");
                return errno;
            }
            int m = atoi(nr); //fac citirea linie cu linie
            for (int j = 0; j < m; j++)
            {
                for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - ShowTop. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                printf("\n");
            }
        }
        else if ((strcmp(com, "Vote") == 0) && (login == 1)) //VOTE
        {
            //afisez topul curent
            printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
            char nr[10];
            if (read(sd, nr, 10) < 0)
            {
                perror("[client]Read error - Vote. \n");
                return errno;
            }
            int m = atoi(nr);
            for (int j = 0; j < m; j++)
            {
                for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - Vote. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                printf("\n");
            }
            //utilizatorul alege daca vrea sau nu sa voteze
            char r[1];
            int ok = 0;
            while (ok == 0)
            {
                printf("\033[01;33m Would you like to vote? (Y/N): \033[0m");
                fflush(stdout);
                bzero(r, 1);
                scanf("%s", r);
                if ((strcmp(r, "Y") != 0) && (strcmp(r, "N") != 0))
                    printf("\033[1;31m The allowed answers are Y / N! \n\033[0m");
                else if ((strcmp(r, "Y") == 0) || (strcmp(r, "N") == 0))
                    ok++;
            }
            if (write(sd, r, 1) <= 0)
            {
                perror("[client]Write error - Vote - raspuns.\n");
                return errno;
            }
            if (strcmp(r, "N") == 0)
                printf("\n\033[01;33m Thank you! \n\033[0m");
            if (strcmp(r, "Y") == 0) //daca se vrea votarea
            {
                //verificam permisiunea de vot
                char permisiune[1];
                if (read(sd, permisiune, 1) < 0)
                {
                    perror("[client]Read error - Vote. \n");
                    return errno;
                }
                if (permisiune[0] == '1') //daca utilizatorul poate vota
                {
                    //colectam datele melodiei, precum si nota cu care se va finaliza votul
                    char titlu[50];
                    char artist[50];
                    char nota[2];
                    printf("\033[01;33m Give the title of the song: \033[0m");
                    fflush(stdout);
                    scanf(" %[^\t\n]s", titlu);
                    if (write(sd, titlu, 50) <= 0)
                    {
                        perror("[client]Write error - Vote - title.\n");
                        return errno;
                    }
                    printf("\033[01;33m Give the singer of the song: \033[0m");
                    fflush(stdout);
                    scanf(" %[^\t\n]s", artist);
                    if (write(sd, artist, 50) <= 0)
                    {
                        perror("[client]Write error - Vote - artist.\n");
                        return errno;
                    }
                    int k = 0;
                    while (k == 0)
                    {
                        bzero(nota, 2);
                        printf("\033[01;33m Give your vote for the song (a number between 1 and 10): \033[0m");
                        fflush(stdout);
                        scanf(" %[^\t\n]s", nota);
                        int nr = atoi(nota);
                        if (nr >= 1 && nr <= 10)
                            k++;
                        else
                            printf("\033[1;31m Invalid vote, try again... \n\033[0m");
                    }
                    if (write(sd, nota, 2) <= 0)
                    {
                        perror("[client]Write error - Vote - nota.\n");
                        return errno;
                    }
                    char exista[10];
                    bzero(exista, 10);
                    if (read(sd, exista, 10) < 0)
                    {
                        perror("[client]Read error - Vote. \n");
                        return errno;
                    }
                    if (strcmp(exista, "NULL") != 0)
                    {
                        //verificam daca votul a fost sau nu finalizat
                        char c[10];
                        bzero(c, 10);
                        if (read(sd, c, 10) < 0)
                        {
                            perror("[client]Read error - Vote. \n");
                            return errno;
                        }
                        if (c[0] == '1')
                            fprintf(stdout, "\033[01;33m Records created successfully \n\033[0m");
                        else
                        {
                            fprintf(stdout, "\033[1;31m Records couldn't be created \n\033[0m");
                            continue;
                        }
                        //afisam noul top
                        printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
                        char nr[10];
                        if (read(sd, nr, 10) < 0)
                        {
                            perror("[client]Read error - Vote. \n");
                            return errno;
                        }
                        int m = atoi(nr);
                        for (int j = 0; j < m; j++)
                        {
                            for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                            {
                                char melodie[1000];
                                if (read(sd, melodie, 1000) < 0)
                                {
                                    perror("[client]Read error - Vote. \n");
                                    return errno;
                                }
                                printf("%s\n", melodie);
                            }
                            printf("\n");
                        }
                    }
                    else
                        printf("\n\033[1;31m Song does not exist... \n\033[0m");
                }
                else
                    printf("\n\033[1;31m Permission denied... \n\033[0m"); //utilizatorul nu are permisiunea de a vota
            }
        }
        else if ((strcmp(com, "ModifyUserPermission") == 0) && (login == 1)) //MODIFYUSERPERMISSION
        {
            char d[10]; //va vedea tipul utilizatorului (user / admin)
            if (read(sd, d, 10) < 0)
            {
                perror("[client]Read error - ModifyUserPermission. \n");
                return errno;
            }
            if (strcmp(d, "admin") == 0) //daca utilizatorul e de tip admin
            {
                //afisam userii cu permisiunile curente
                printf("\n\033[1;32m USERS: \n\033[0m");
                fflush(stdout);
                char nr[10];
                if (read(sd, nr, 10) < 0)
                {
                    perror("[client]Read error - ModifyUserPermission. \n");
                    return errno;
                }
                int m = atoi(nr);
                for (int j = 0; j < m; j++)
                {
                    for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                    {
                        char melodie[1000];
                        if (read(sd, melodie, 1000) < 0)
                        {
                            perror("[client]Read error - ModifyUserPermission. \n");
                            return errno;
                        }
                        printf("%s\n", melodie);
                    }
                    printf("\n");
                }
                char exista[10];
                int verif = 0;
                while (verif == 0)
                {
                    //citim de la tastatura si trimitem serverului numele utilizatorului care va avea permisiunile modificare
                    printf("\033[01;33m Give the username of the account you want to modify: \033[0m");
                    fflush(stdout);
                    char nume[10];
                    bzero(nume, 10);
                    scanf("%s", nume);
                    if (write(sd, nume, 10) <= 0)
                    {
                        perror("[server]Write error - ModifyUserPermission.\n");
                        continue; /* continuam sa ascultam */
                    }
                    //verificam daca utilizatorul exista in baza de date
                    bzero(exista, 10); //va vedea daca utilizatorul dat e sau nu in tabela
                    if (read(sd, exista, 10) < 0)
                    {
                        perror("[client]Read error - ModifyUserPermission. \n");
                        return errno;
                    }
                    if (exista[0] == '1' || exista[0] == '0') //daca ascesta exista
                    {
                        //efectuam modificarile dorite in server, cu date oferite din client
                        int ok = 0;
                        char option[1];
                        while (ok == 0)
                        {
                            printf("\033[01;33m Give or Take permission? (G/T): \033[0m");
                            fflush(stdout);
                            bzero(option, 1);
                            scanf("%s", option);
                            if ((strcmp(option, "G") != 0) && (strcmp(option, "T") != 0))
                                printf("\033[1;31m The allowed answers are G / T! \n\033[0m");
                            else if ((strcmp(option, "G") == 0) || (strcmp(option, "T") == 0))
                                ok++;
                        }
                        if (write(sd, option, 1) <= 0)
                        {
                            perror("[server]Write error - ModifyUserPermission.\n");
                            continue; /* continuam sa ascultam */
                        }
                        char e[10];
                        bzero(e, 10);
                        if (read(sd, e, 10) < 0)
                        {
                            perror("[client]Read error - ModifyUserPermission. \n");
                            return errno;
                        }
                        if (e[0] == '1')
                            fprintf(stdout, "\n\033[01;33m Records created successfully \n\033[0m");
                        else
                        {
                            fprintf(stdout, "\n\033[1;31m Records couldn't be created \n\033[0m");
                            continue;
                        }
                        //afisam noua lista cu utilizatori
                        printf("\n\033[1;32m USERS: \n\033[0m");
                        fflush(stdout);
                        for (int j = 0; j < m; j++)
                        {
                            for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                            {
                                char melodie[1000];
                                if (read(sd, melodie, 1000) < 0)
                                {
                                    perror("[client]Read error - ModifyUserPermission. \n");
                                    return errno;
                                }
                                printf("%s\n", melodie);
                            }
                            printf("\n");
                        }
                        verif++;
                    }
                    else
                        printf("\n\n\033[1;31m User doesn't exist! \n\033[0m"); //user-ul la care vrem sa modificam permisiunea nu exista
                }
            }
            else
                printf("\n\033[1;31m Permission denied... You are not admin! \n\033[0m"); //user-ul nu are dreptul sa modifice date
        }
        else if ((strcmp(com, "DeleteSong") == 0) && (login == 1)) //DELETESONG
        {
            char d[10]; //va vedea tipul utilizatorului (user / admin)
            if (read(sd, d, 10) < 0)
            {
                perror("[client]Read error - DeleteSong. \n");
                return errno;
            }
            if (strcmp(d, "admin") == 0) //daca utilizatorul e de tip admin
            {
                //afisam melodiile curente si rankul acestora
                printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
                char nr[10];
                if (read(sd, nr, 10) < 0)
                {
                    perror("[client]Read error - DeleteSong. \n");
                    return errno;
                }
                int m = atoi(nr);
                for (int j = 0; j < m; j++)
                {
                    for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                    {
                        char melodie[1000];
                        if (read(sd, melodie, 1000) < 0)
                        {
                            perror("[client]Read error - DeleteSong. \n");
                            return errno;
                        }
                        printf("%s\n", melodie);
                    }
                    printf("\n");
                }
                char exista[10];
                //citim de la tastatura si trimitem serverului titlul si artistul melodiei care va fi sterse
                printf("\033[01;33m Give the title of the song you want to delete: \033[0m");
                fflush(stdout);
                char titlu[10];
                bzero(titlu, 10);
                scanf(" %[^\t\n]s", titlu);
                if (write(sd, titlu, 10) <= 0)
                {
                    perror("[server]Write error - DeleteSong.\n");
                    continue; /* continuam sa ascultam */
                }
                printf("\033[01;33m Give the artist of the song you want to delete: \033[0m");
                fflush(stdout);
                char artist[10];
                bzero(artist, 10);
                scanf(" %[^\t\n]s", artist);
                if (write(sd, artist, 10) <= 0)
                {
                    perror("[server]Write error - DeleteSong.\n");
                    continue; /* continuam sa ascultam */
                }
                //verificam daca melodia exista in baza de date
                bzero(exista, 10); //va vedea daca melodia data e sau nu in tabela
                if (read(sd, exista, 10) < 0)
                {
                    perror("[client]Read error - DeleteSong. \n");
                    return errno;
                }
                if (strcmp(exista, "NULL") != 0) //daca asceasta exista
                {
                    //efectuam modificarile dorite in server, cu date oferite din client
                    char e[10];
                    bzero(e, 10);
                    if (read(sd, e, 10) < 0)
                    {
                        perror("[client]Read error - DeleteSong. \n");
                        return errno;
                    }
                    if (e[0] == '1')
                        fprintf(stdout, "\n\033[01;33m Song removed successfully \n\033[0m");
                    else
                    {
                        fprintf(stdout, "\n\033[1;31m Song couldn't be deleted \n\033[0m");
                        continue;
                    }
                    printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
                    for (int j = 0; j < m - 1; j++)
                    {
                        for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                        {
                            char melodie[1000];
                            if (read(sd, melodie, 1000) < 0)
                            {
                                perror("[client]Read error - DeleteSong. \n");
                                return errno;
                            }
                            printf("%s\n", melodie);
                        }
                        printf("\n");
                    }
                }
                else
                    printf("\n\033[1;31m Song doesn't exist! \n\033[0m");
            }
            else
                printf("\n\033[1;31m Permission denied... You are not admin! \n\033[0m"); //user-ul nu are dreptul sa modifice date
        }
        else if ((strcmp(com, "Info") == 0) && (login == 1)) //INFO
        {
            //afisez topul curent
            printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
            char nr[10];
            if (read(sd, nr, 10) < 0)
            {
                perror("[client]Read error - Info. \n");
                return errno;
            }
            int m = atoi(nr);
            for (int j = 0; j < m; j++)
            {
                for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - Info. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                printf("\n");
            }
            char exista[10];
            //citim de la tastatura si trimitem serverului titlul si artistul melodiei care va fi sterse
            printf("\033[01;33m Give the title of the song: \033[0m");
            fflush(stdout);
            char titlu[50];
            bzero(titlu, 50);
            scanf(" %[^\t\n]s", titlu);
            if (write(sd, titlu, 50) <= 0)
            {
                perror("[server]Write error - Info.\n");
                continue; /* continuam sa ascultam */
            }
            printf("\033[01;33m Give the artist of the song: \033[0m");
            fflush(stdout);
            char artist[50];
            bzero(artist, 50);
            scanf(" %[^\t\n]s", artist);
            if (write(sd, artist, 50) <= 0)
            {
                perror("[server]Write error - Info.\n");
                continue; /* continuam sa ascultam */
            }
            //verificam daca melodia exista in baza de date
            bzero(exista, 10); //va vedea daca melodia data e sau nu in tabela
            if (read(sd, exista, 10) < 0)
            {
                perror("[client]Read error - Info. \n");
                return errno;
            }
            if (strcmp(exista, "NULL") != 0) //daca aceasta exista
            {
                printf("\033[01;33m     INFORMATION: \n\033[0m");
                for (int i = 0; i < 6; i++) //6 fiind numarul de coloane din tabela music care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - Info. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                //primesc din server numarul de comentarii asociate piesei respective si apoi le afisez
                char comment[10];
                if (read(sd, comment, 10) < 0)
                {
                    perror("[client]Read error - Info. \n");
                    return errno;
                }
                int loop = atoi(comment);
                if (loop != 0)
                {
                    printf("\033[01;33m     COMMENTS: \n\033[0m");
                    for (int i = 0; i < loop * 2; i++) //6 fiind numarul de coloane din tabela music care se afiseaza
                    {
                        char comm[1000];
                        if (read(sd, comm, 1000) < 0)
                        {
                            perror("[client]Read error - Info. \n");
                            return errno;
                        }
                        printf("%s\n", comm);
                    }
                }
                else
                    printf("\033[01;33m     THERE ARE NO COMMENTS. \n\033[0m");
            }
            else
                printf("\n\033[1;31m Song doesn't exist! \n\033[0m");
        }
        else if ((strcmp(com, "AddComment") == 0) && (login == 1)) //VOTE
        {
            //afisez topul curent
            printf("\n\033[1;32m CURRENT TOP: \n\033[0m");
            char nr[10];
            if (read(sd, nr, 10) < 0)
            {
                perror("[client]Read error - AddComment. \n");
                return errno;
            }
            int m = atoi(nr);
            for (int j = 0; j < m; j++)
            {
                for (int i = 0; i < 3; i++) //3 fiind numarul de coloane din tabela music care se afiseaza
                {
                    char melodie[1000];
                    if (read(sd, melodie, 1000) < 0)
                    {
                        perror("[client]Read error - AddComment. \n");
                        return errno;
                    }
                    printf("%s\n", melodie);
                }
                printf("\n");
            }
            //colectam datele melodiei
            char titlu[50];
            char artist[50];
            printf("\033[01;33m Give the title of the song: \033[0m");
            fflush(stdout);
            scanf(" %[^\t\n]s", titlu);
            if (write(sd, titlu, 50) <= 0)
            {
                perror("[client]Write error - AddComment - title.\n");
                return errno;
            }
            printf("\033[01;33m Give the singer of the song: \033[0m");
            fflush(stdout);
            scanf(" %[^\t\n]s", artist);
            if (write(sd, artist, 50) <= 0)
            {
                perror("[client]Write error - AddComment - artist.\n");
                return errno;
            }
            //retine daca melodia exista sau nu deja in top
            char exista[10];
            bzero(exista, 10);
            if (read(sd, exista, 10) < 0)
            {
                perror("[client]Read error - AddComment. \n");
                return errno;
            }
            if (strcmp(exista, "NULL") != 0) //exista
            {
                char comment[500];
                printf("\033[01;33m Your comment: \033[0m");
                fflush(stdout);
                scanf(" %[^\t\n]s", comment);
                if (write(sd, comment, 500) <= 0)
                {
                    perror("[client]Write error - AddComment.\n");
                    return errno;
                }
                //verificam daca adaugarea datelor a fost sau nu finalizata
                char c[10];
                bzero(c, 10);
                if (read(sd, c, 10) < 0)
                {
                    perror("[client]Read error - AddComment. \n");
                    return errno;
                }
                if (c[0] == '1')
                    fprintf(stdout, "\033[01;33m Records created successfully \n\033[0m");
                else
                {
                    fprintf(stdout, "\033[1;31m Records couldn't be created \n\033[0m");
                    continue;
                }
            }
            else
                printf("\n\033[1;31m Song does not exist... \n\033[0m");

        }
        else if (strcmp(com, "exit") == 0) //EXIT
        {
            printf("\n\033[01;33m Logging off... \n\033[0m");
            close(sd);
            exit(1);
            login = 0;
        }
        else
        {
            printf("\n\033[1;31m Invalid command... \n\033[0m");
            continue;
        }
    }
}
