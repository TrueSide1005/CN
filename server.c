//Folosim baze de date din SQLite ("date"), contine tabelele: users, music, comments, gen
//Pentru rulare gcc -Wall server.c -o server.exe -lsqlite3
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

char res[10], vot[1];    //retin rezultate din functiile de tip callback
char usr[100], pas[100]; //vor retine usernameul si respectiv parola
int aparitie = 0;        //retine daca utilizatorul e logat sau nu

/* folosita in login, NewSong*/
static int callback1(void *data, int argc, char **argv, char **azColName)
{
    int i;
    bzero(res, 10);
    for (i = 0; i < argc; i++)
    {
        sprintf(res, "%s", argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

char melodie[1000];

void info(char sql[100])   //afiseaza o singura informatie dintr-o tabela
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    void *data;
    /* Open database */
    rc = sqlite3_open("data", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    rc = sqlite3_exec(db, sql, callback1, data, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);
}

static int callback2(void *data, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        bzero(melodie, 1000);
        sprintf(melodie, "\033[1;34m %s:\033[0m %s", azColName[i], argv[i] ? argv[i] : "NULL");
        if (write(data, melodie, 1000) <= 0)
        {
            perror("[server]Write error - login.\n");
            continue; /* continuam sa ascultam */
        }
    }
    return 0;
}

void top1(int client) //afiseaza topul melodiilor in functie de nr de voturi
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    void *data = client;
    /* Open database */
    rc = sqlite3_open("data", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    char sql1[100] = "SELECT rank, titlu, artist FROM music ORDER BY rank DESC;";
    rc = sqlite3_exec(db, sql1, callback2, data, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);
}

void useri(int client) //afiseaza continutul tabelei user, fara date private cum ar fi parola
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    void *data = client;
    /* Open database */
    rc = sqlite3_open("data", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    }
    char sql1[100] = "SELECT username, tip, vot FROM users;";
    rc = sqlite3_exec(db, sql1, callback2, data, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;
    char msg[100];         //mesajul primit de la client
    char msgrasp[1] = " "; //mesaj de raspuns pentru client
    int sd;                //descriptorul de socket

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;                // stabilirea familiei de socket-uri
    server.sin_addr.s_addr = htonl(INADDR_ANY); // acceptam orice adresa
    server.sin_port = htons(PORT);              // utilizam un port utilizator

    // atasam socketul
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Bind error.\n");
        return errno;
    }

    // ascultam daca vin clientii sa se conecteze
    if (listen(sd, 10) == -1)
    {
        perror("[server]Listen error.\n");
        return errno;
    }

    // servim clientii in mod concurent
    while (1)
    {
        int client;
        int length = sizeof(from);
        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);
        // acceptam un client (stare blocanta pana la realizarea conexiunii)
        client = accept(sd, (struct sockaddr *)&from, &length);
        // eroare la acceptarea conexiunii cu un client
        if (client < 0)
        {
            perror("[server]Accept error.\n");
            continue;
        }
        int pid;
        if ((pid = fork()) == -1) //eroare la fork
        {
            close(client);
            continue;
        }
        else if (pid > 0) //parinte
        {
            close(client);
            continue;
        }
        else if (pid == 0) //copil
        {
            while (1)
            {
                bzero(msg, 100); //il pregatesc
                printf("[server]Awaiting command...\n");
                fflush(stdout);
                // citesc comanda
                if (read(client, msg, 100) <= 0)
                {
                    char str[INET_ADDRSTRLEN];
                    printf("\nClient %s disconnected \n", inet_ntop(AF_INET, &from, str, INET_ADDRSTRLEN));
                    close(client);
                    break;
                    continue; //continui sa ascult
                }
                printf("[server]Message recieved...%s\n", msg);
                if (strcmp(msg, "login") == 0 && (aparitie == 0)) //LOGIN
                {
                    bzero(usr, 100);
                    if (read(client, usr, 100) <= 0) //citeste username-ul
                    {
                        perror("[server]Read error login.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    bzero(pas, 100);
                    if (read(client, pas, 100) <= 0) //citeste parola
                    {
                        perror("[server]Read error login.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    char sql[100];
                    //vom selecta datele de pe coloana vot pentru a vedea daca user-ul si parola sunt prezente in tabela users
                    sprintf(sql, "SELECT vot FROM users WHERE username='%s' AND password='%s';", usr, pas);
                    strcpy(res, "NULL");
                    info(sql);
                    aparitie = 0;
                    if (strcmp(res, "NULL") != 0) //datele sunt regasite in tabel
                    {
                        aparitie++;      //tinem minte ca utilizatorul e logat
                        vot[0] = res[0]; //memoram si permisiunea acestuia de a vota pe care o vom folosi in Vote
                    }
                    bzero(msgrasp, 1); //trimit in client daca utilizatorul exista sau nu
                    if (aparitie != 0)
                        strcpy(msgrasp, "1");
                    else
                        strcpy(msgrasp, "0");
                    if (write(client, msgrasp, 1) <= 0)
                    {
                        perror("[server]Write error - login.\n");
                        continue; /* continuam sa ascultam */
                    }
                }
                else if ((strcmp(msg, "Join") == 0) && (aparitie == 0))
                {
                    char nume[100], parola[100], cont[1];
                    //citesc numele dat de clien si verific daca exista in baza de date
                    bzero(nume, 100);
                    if (read(client, nume, 100) <= 0)
                    {
                        perror("[server]Read error Join.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    strcpy(res, "NULL");
                    char sql[100];
                    sprintf(sql, "SELECT ID FROM users WHERE username='%s';", nume);
                    info(sql);
                    bzero(cont, 0);
                    if (strcmp(res, "NULL") == 0)
                        strcpy(cont, "0");
                    else
                        strcpy(cont, "1");
                    if (write(client, cont, 1) <= 0)
                    {
                        perror("[server]Write error - Join.\n");
                        continue; /* continuam sa ascultam */
                    }
                    if (cont[0] == '0') //exista
                    {
                        bzero(parola, 100);
                        if (read(client, parola, 100) <= 0) //citesc parola
                        {
                            perror("[server]Read error Join.\n");
                            close(client);
                            continue; //continui sa ascult
                        }
                        char s[100] = "SELECT MAX(ID) FROM users;";
                        bzero(res, 10);
                        info(s);
                        int nr = atoi(res);
                        char ss[100];
                        //inserez noul utilizator
                        sprintf(ss, "INSERT INTO users (ID, username, password, tip, vot) VALUES ('%d','%s', '%s', 'urs', '1');", nr + 1, nume, parola);
                        sqlite3 *db;
                        char *err_msg = 0;
                        int rc;
                        void *data;
                        /* Open database */
                        rc = sqlite3_open("data", &db);
                        if (rc)
                        {
                            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                            return (0);
                        }
                        int rc1 = sqlite3_exec(db, ss, callback1, 0, &err_msg);
                        bzero(msgrasp, 1);
                        if (rc1 != SQLITE_OK)
                        {
                            fprintf(stderr, "SQL error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                            strcpy(msgrasp, "0");
                        }
                        //trimit in client rezultatul (a fost sau nu adaugat utilizatorul)
                        else
                        {
                            fprintf(stdout, "User created successfully\n");
                            strcpy(msgrasp, "1");
                        }
                        if (write(client, msgrasp, 1) <= 0)
                        {
                            perror("[server]Write error - Join.\n");
                            continue; /* continuam sa ascultam */
                        }
                        sqlite3_close(db);
                    }
                }
                else if ((strcmp(msg, "NewSong") == 0) && (aparitie != 0)) //NewSong
                {
                    char sql[100] = "SELECT MAX(nr) FROM music"; //aflu ultima valoare din tabela music pentru a insera o noua melodie la finalul acesteia
                    info(sql);
                    int nr = atoi(res);
                    char title[50], artist[50], link[100], desc[300], gen[40];
                    //obtin datele din client
                    bzero(title, 50);
                    if (read(client, title, 50) <= 0)
                    {
                        perror("[server]Read error NewSong.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    bzero(artist, 50);
                    if (read(client, artist, 50) <= 0)
                    {
                        perror("[server]Read error NewSong.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    bzero(link, 100);
                    if (read(client, link, 100) <= 0)
                    {
                        perror("[server]Read error NewSong.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    bzero(desc, 300);
                    if (read(client, desc, 300) <= 0)
                    {
                        perror("[server]Read error NewSong.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    bzero(gen, 40);
                    if (read(client, gen, 40) <= 0)
                    {
                        perror("[server]Read error NewSong.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    for (int i = 0; i < strlen(title); i++)
                        title[i] = toupper(title[i]);
                    for (int i = 0; i < strlen(artist); i++)
                        artist[i] = toupper(artist[i]);
                    char exista[10];
                    //verific daca piesa exista deja in top
                    strcpy(res, "NULL");
                    char ss[100];
                    sprintf(ss, "SELECT nr FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", title, artist);
                    info(ss);
                    strcpy(exista, res);
                    if (write(client, exista, 10) <= 0)
                    {
                        perror("[server]Write error - NewSong.\n");
                        continue; /* continuam sa ascultam */
                    }
                    if (strcmp(exista, "NULL") == 0)
                    {
                        for (int i = 1; i < strlen(title); i++)
                        {
                            title[i] = tolower(title[i]);
                            if (title[i - 1] == ' ')
                                title[i] = toupper(title[i]);
                        }
                        for (int i = 1; i < strlen(artist); i++)
                        {
                            artist[i] = tolower(artist[i]);
                            if (artist[i - 1] == ' ')
                                artist[i] = toupper(artist[i]);
                        }
                        //actualizez tablea
                        sqlite3 *db;
                        char *err_msg = 0;
                        int rc;
                        void *data;
                        /* Open database */
                        rc = sqlite3_open("data", &db);
                        if (rc)
                        {
                            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                            return (0);
                        }
                        int sql1[1000];
                        sprintf(sql1, "INSERT INTO music (nr,titlu,artist,link,description,gen) VALUES ('%d','%s', '%s', '%s', '%s', '%s' );", nr + 1, title, artist, link, desc, gen);
                        int rc1 = sqlite3_exec(db, sql1, callback1, 0, &err_msg);
                        bzero(msgrasp, 1);
                        if (rc1 != SQLITE_OK)
                        {
                            fprintf(stderr, "SQL error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                            strcpy(msgrasp, "0");
                        }
                        //trimit in client rezultatul (a fost sau nu adaugata piesa)
                        else
                        {
                            fprintf(stdout, "Records created successfully\n");
                            strcpy(msgrasp, "1");
                        }
                        if (write(client, msgrasp, 1) <= 0)
                        {
                            perror("[server]Write error - NewSong.\n");
                            continue; /* continuam sa ascultam */
                        }
                        sqlite3_close(db);
                    }
                }
                else if ((strcmp(msg, "ShowGenreTop") == 0) && (aparitie != 0)) //SHOWGENRETOP
                {
                    char genre[10];
                    bzero(genre, 10);
                    if (read(client, genre, 10) < 0) //citesc genul dorit
                    {
                        perror("[server]Read error ShowGenreTop.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    for (int i = 0; i < strlen(genre); i++)
                        genre[i] = toupper(genre[i]);
                    char sql[100];
                    //aflu si trimit clientului cate melodii apartin genului
                    sprintf(sql, "SELECT COUNT(*) FROM music WHERE UPPER(gen) LIKE '%%%s%%'", genre);
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - ShowGenreTop.\n");
                        continue; // continuam sa ascultam
                    }
                    //extrag datele din baza si le trimit clientului linie cu linie
                    sqlite3 *db;
                    char *err_msg = 0;
                    int rc;
                    void *data = client;
                    // Open database
                    rc = sqlite3_open("data", &db);
                    if (rc)
                    {
                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                        return (0);
                    }
                    char sql1[100];
                    bzero(sql1, 100);
                    for (int i = 0; i < strlen(genre); i++)
                        genre[i] = toupper(genre[i]);
                    sprintf(sql1, "SELECT rank, titlu, artist FROM music WHERE UPPER(gen) LIKE '%%%s%%' ORDER BY rank DESC;", genre);
                    rc = sqlite3_exec(db, sql1, callback2, data, &err_msg);
                    if (rc != SQLITE_OK)
                    {
                        fprintf(stderr, "SQL error: %s\n", err_msg);
                        sqlite3_free(err_msg);
                    }
                    sqlite3_close(db);
                }
                else if ((strcmp(msg, "ShowTop") == 0) && (aparitie != 0)) //SHOWTOP
                {
                    //trimit numarul de melodii din top
                    char sql[100] = "SELECT MAX(nr) FROM music";
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - ShowGenreTop.\n");
                        continue; // continuam sa ascultam
                    }
                    //functia top1 afiseaza topul pieselor
                    top1(client); //functia va trimite, linie cu linie, topul catre client
                }
                else if ((strcmp(msg, "Vote") == 0) && (aparitie != 0)) //VOTE
                {
                    //afisez topul initial
                    char sql[100] = "SELECT MAX(nr) FROM music";
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - Vote.\n");
                        continue; // continuam sa ascultam
                    }
                    //functia top1 afiseaza topul pieselor
                    top1(client);
                    char raspuns[1];
                    bzero(raspuns, 1);
                    if (read(client, raspuns, 1) <= 0) //raspuns retine daca clientul vrea sau nu sa voteze
                    {
                        perror("[server]Read error Vote.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    if (raspuns[0] == 'Y') //daca se voteaza
                    {
                        char vot1[10];
                        char sql2[100];
                        sprintf(sql2, "SELECT vot FROM users WHERE username='%s' AND password='%s';", usr, pas);
                        strcpy(res, "NULL");
                        info(sql2);
                        strcpy(vot1, res);
                        if (write(client, vot1, 1) <= 0) //trimit permisiunea de mod si in client
                        {
                            perror("[server]Write error - Vote.\n");
                            continue; /* continuam sa ascultam */
                        }
                        if (vot1[0] == '1') //daca userul are permisiunea sa voteze
                        {
                            //retin datele melodiei care va fi votate
                            char titlu[50];
                            char artist[50];
                            char nota[2];
                            bzero(titlu, 50);
                            bzero(artist, 50);
                            bzero(nota, 2);
                            if (read(client, titlu, 50) <= 0)
                            {
                                perror("[server]Read error Vote.\n");
                                close(client);
                                continue; //continui sa ascult
                            }
                            if (read(client, artist, 50) <= 0)
                            {
                                perror("[server]Read error Vote.\n");
                                close(client);
                                continue; //continui sa ascult
                            }
                            if (read(client, nota, 2) <= 0)
                            {
                                perror("[server]Read error Vote.\n");
                                close(client);
                                continue; //continui sa ascult
                            }
                            for (int i = 0; i < strlen(titlu); i++)
                                titlu[i] = toupper(titlu[i]);
                            for (int i = 0; i < strlen(artist); i++)
                                artist[i] = toupper(artist[i]);
                            char exista[10];
                            strcpy(res, "NULL");
                            char ss[100];
                            sprintf(ss, "SELECT nr FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                            info(ss);
                            strcpy(exista, res);
                            if (write(client, exista, 10) <= 0)
                            {
                                perror("[server]Write error - Vote.\n");
                                continue; /* continuam sa ascultam */
                            }
                            if (strcmp(exista, "NULL") != 0)
                            {
                                //modific campul rank pentru melodia aleasa, adaugand valoarea data
                                sqlite3 *db;
                                char *err_msg = 0;
                                int rc;
                                void *data;
                                /* Open database */
                                rc = sqlite3_open("data", &db);
                                if (rc)
                                {
                                    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                                    return (0);
                                }
                                int nr = atoi(nota);
                                char s[100];
                                sprintf(s, "SELECT rank FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                                info(s);
                                int x = atoi(res);
                                nr = nr + x;
                                char sql[100];
                                sprintf(sql, "UPDATE music SET rank='%d' WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", nr, titlu, artist);
                                rc = sqlite3_exec(db, sql, callback1, 0, &err_msg);
                                if (rc != SQLITE_OK)
                                {
                                    fprintf(stderr, "SQL error: %s\n", err_msg);
                                    sqlite3_free(err_msg);
                                    strcpy(msgrasp, "0");
                                }
                                else
                                {
                                    fprintf(stdout, "Records created successfully\n");
                                    strcpy(msgrasp, "1");
                                }
                                if (write(client, msgrasp, 1) <= 0)
                                {
                                    perror("[server]Write error - Vote.\n");
                                    continue; /* continuam sa ascultam */
                                }
                                char sq[100] = "SELECT MAX(nr) FROM music";
                                info(sq);
                                if (write(client, res, 10) <= 0)
                                {
                                    perror("[server]Write error - Vote.\n");
                                    continue; // continuam sa ascultam
                                }
                                //functia top1 afiseaza topul pieselor
                                top1(client);
                                sqlite3_close(db);
                            }
                        }
                    }
                }
                else if ((strcmp(msg, "ModifyUserPermission") == 0) && (aparitie != 0)) //MODIFYUSERPERRMISION
                {
                    char sql[100];
                    //vom selecta datele de pe coloana tip pentru a vedea daca user-ul e admin
                    sprintf(sql, "SELECT tip FROM users WHERE username='%s' AND password='%s';", usr, pas);
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - ModifyUserPermission.\n");
                        continue; /* continuam sa ascultam */
                    }
                    if (strcmp(res, "admin") == 0) //utilizatorul e admin, poate face modificari
                    {
                        //afisez tabela cu userii, fara a divulga parola
                        char s[100] = "select count(*) from users where ID  IS NOT NULL";
                        info(s);
                        if (write(client, res, 10) <= 0)
                        {
                            perror("[server]Write error - ModifyUserPermission.\n");
                            continue; // continuam sa ascultam
                        }
                        useri(client);
                        int verif = 0;
                        while (verif == 0)
                        {
                            //preiau numele de utilizator la care voi modifica
                            char nume[10];
                            bzero(nume, 10);
                            if (read(client, nume, 10) < 0)
                            {
                                perror("[client]Read error - ModifyUserPermission. \n");
                                return errno;
                            }
                            //verific daca utilizatorul exista
                            sprintf(sql, "SELECT vot FROM users WHERE username='%s';", nume);
                            strcpy(res, "NULL");
                            //Execut comanda SQL
                            info(sql);
                            if (write(client, res, 10) <= 0)
                            {
                                perror("[server]Write error - ModifyUserPermission.\n");
                                continue; /* continuam sa ascultam */
                            }
                            //daca utilizatorul exista, modific dreptul de vot
                            if (strcmp(res, "NULL") != 0)
                            {
                                verif++;
                                char option[1];
                                bzero(option, 1);
                                if (read(client, option, 1) < 0)
                                {
                                    perror("[client]Read error - ModifyUserPermission. \n");
                                    return errno;
                                }

                                if (option[0] == 'G') //este data permisiunea
                                {
                                    sqlite3 *db;
                                    char *err_msg = 0;
                                    int rc;
                                    void *data;
                                    /* Open database */
                                    rc = sqlite3_open("data", &db);
                                    if (rc)
                                    {
                                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                                        return (0);
                                    }
                                    char sql[100];
                                    sprintf(sql, "UPDATE users SET vot='1' WHERE username='%s';", nume);
                                    rc = sqlite3_exec(db, sql, callback1, 0, &err_msg);
                                    if (rc != SQLITE_OK)
                                    {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        strcpy(msgrasp, "0");
                                    }
                                    else
                                    {
                                        fprintf(stdout, "Records created successfully\n");
                                        strcpy(msgrasp, "1");
                                    }
                                    if (write(client, msgrasp, 1) <= 0)
                                    {
                                        perror("[server]Write error - ModifyUserPermission.\n");
                                        continue; /* continuam sa ascultam */
                                    }
                                    sqlite3_close(db);
                                    //afisez iar lista cu utilizatorii, modificata
                                    useri(client);
                                }
                                if (option[0] == 'T') //permisiunea este luata
                                {
                                    sqlite3 *db;
                                    char *err_msg = 0;
                                    int rc;
                                    void *data;
                                    /* Open database */
                                    rc = sqlite3_open("data", &db);
                                    if (rc)
                                    {
                                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                                        return (0);
                                    }
                                    char sql[100];
                                    sprintf(sql, "UPDATE users SET vot='0' WHERE username='%s';", nume);
                                    rc = sqlite3_exec(db, sql, callback1, 0, &err_msg);
                                    if (rc != SQLITE_OK)
                                    {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        strcpy(msgrasp, "0");
                                    }
                                    else
                                    {
                                        fprintf(stdout, "Records created successfully\n");
                                        strcpy(msgrasp, "1");
                                    }
                                    if (write(client, msgrasp, 1) <= 0)
                                    {
                                        perror("[server]Write error - ModifyUserPermission.\n");
                                        continue; /* continuam sa ascultam */
                                    }
                                    sqlite3_close(db);
                                    //afisez userii, actualizati
                                    useri(client);
                                }
                            }
                        }
                    }
                }
                else if ((strcmp(msg, "DeleteSong") == 0) && (aparitie != 0)) //DELETESONG
                {
                    char sql[100];
                    //vom selecta datele de pe coloana tip pentru a vedea daca user-ul e admin
                    sprintf(sql, "SELECT tip FROM users WHERE username='%s' AND password='%s';", usr, pas);
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - DeleteSong.\n");
                        continue; /* continuam sa ascultam */
                    }
                    if (strcmp(res, "admin") == 0) //utilizatorul e admin, poate face modificari
                    {
                        //afisez topul
                        char sql[100] = "SELECT MAX(nr) FROM music";
                        info(sql);
                        if (write(client, res, 10) <= 0)
                        {
                            perror("[server]Write error - DeleteSong.\n");
                            continue; // continuam sa ascultam
                        }
                        //functia top1 afiseaza topul pieselor
                        top1(client);
                        int verif = 0;
                        //preiau titlul si artistul melodiei de sters
                        char titlu[10];
                        bzero(titlu, 10);
                        if (read(client, titlu, 10) < 0)
                        {
                            perror("[client]Read error - DeleteSong. \n");
                            return errno;
                        }
                        char artist[10];
                        bzero(artist, 10);
                        if (read(client, artist, 10) < 0)
                        {
                            perror("[client]Read error - DeleteSong. \n");
                            return errno;
                        }
                        for (int i = 0; i < strlen(titlu); i++)
                            titlu[i] = toupper(titlu[i]);
                        for (int i = 0; i < strlen(artist); i++)
                            artist[i] = toupper(artist[i]);
                        //verific daca melodia exista
                        sprintf(sql, "SELECT nr FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                        strcpy(res, "NULL");
                        //Execut comanda SQL
                        info(sql);
                        if (write(client, res, 10) <= 0)
                        {
                            perror("[server]Write error - DeleteSong.\n");
                            continue; /* continuam sa ascultam */
                        }
                        //daca melodia exista, o  sterg din top
                        if (strcmp(res, "NULL") != 0)
                        {
                            sqlite3 *db;
                            char *err_msg = 0;
                            int rc;
                            void *data;
                            /* Open database */
                            rc = sqlite3_open("data", &db);
                            if (rc)
                            {
                                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                                return (0);
                            }
                            char sql[100];
                            sprintf(sql, "DELETE FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                            rc = sqlite3_exec(db, sql, callback1, 0, &err_msg);
                            if (rc != SQLITE_OK)
                            {
                                fprintf(stderr, "SQL error: %s\n", err_msg);
                                sqlite3_free(err_msg);
                                strcpy(msgrasp, "0");
                            }
                            else
                            {
                                fprintf(stdout, "Records created successfully\n");
                                strcpy(msgrasp, "1");
                            }
                            if (write(client, msgrasp, 1) <= 0)
                            {
                                perror("[server]Write error - DeleteSong.\n");
                                continue; /* continuam sa ascultam */
                            }
                            sqlite3_close(db);
                            //afisez iar lista cu utilizatorii, modificata
                            top1(client);
                        }
                    }
                }
                else if ((strcmp(msg, "Info") == 0) && (aparitie != 0)) //INFO
                {
                    //afisez topul
                    char sql[100] = "SELECT MAX(nr) FROM music";
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - Info.\n");
                        continue; // continuam sa ascultam
                    }
                    //functia top1 afiseaza topul pieselor
                    top1(client);
                    //preiau titlul si artistul melodiei despre care se vor da informatii
                    char titlu[50];
                    bzero(titlu, 50);
                    if (read(client, titlu, 50) < 0)
                    {
                        perror("[client]Read error - Info. \n");
                        return errno;
                    }
                    char artist[50];
                    bzero(artist, 50);
                    if (read(client, artist, 50) < 0)
                    {
                        perror("[client]Read error - Info. \n");
                        return errno;
                    }
                    for (int i = 0; i < strlen(titlu); i++)
                        titlu[i] = toupper(titlu[i]);
                    for (int i = 0; i < strlen(artist); i++)
                        artist[i] = toupper(artist[i]);
                    //verific daca melodia exista
                    sprintf(sql, "SELECT nr FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                    strcpy(res, "NULL");
                    //Execut comanda SQL
                    info(sql);
                    int nr = atoi(res);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - Info.\n");
                        continue; /* continuam sa ascultam */
                    }
                    //daca melodia exista, trimit datele despre ea catre client
                    if (strcmp(res, "NULL") != 0)
                    {
                        sqlite3 *db;
                        char *err_msg = 0;
                        int rc;
                        void *data = client;
                        /* Open database */
                        rc = sqlite3_open("data", &db);
                        if (rc)
                        {
                            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                            return (0);
                        }
                        char sql1[100];
                        sprintf(sql1, "SELECT titlu, artist, rank, link, description, gen FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                        rc = sqlite3_exec(db, sql1, callback2, data, &err_msg);
                        if (rc != SQLITE_OK)
                        {
                            fprintf(stderr, "SQL error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                        }
                        bzero(sql1, 100);
                        sprintf(sql1, "SELECT COUNT(*) FROM comments where nr='%d';", nr);
                        info(sql1);
                        if (write(client, res, 10) <= 0)
                        {
                            perror("[server]Write error - Info.\n");
                            continue; /* continuam sa ascultam */
                        }
                        bzero(sql1, 100);
                        sprintf(sql1, "select username, com FROM users join comments on users.ID=comments.id join music ON comments.nr=music.nr WHERE music.nr='%d';", nr);
                        rc = sqlite3_exec(db, sql1, callback2, data, &err_msg);
                        if (rc != SQLITE_OK)
                        {
                            fprintf(stderr, "SQL error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                        }
                        sqlite3_close(db);
                    }
                }
                else if ((strcmp(msg, "AddComment") == 0) && (aparitie != 0)) //ADDCOMMENT
                {
                    //afisez topul initial
                    char sql[100] = "SELECT MAX(nr) FROM music";
                    info(sql);
                    if (write(client, res, 10) <= 0)
                    {
                        perror("[server]Write error - AddComment.\n");
                        continue; // continuam sa ascultam
                    }
                    //functia top1 afiseaza topul pieselor
                    top1(client);
                    char titlu[50];
                    char artist[50];
                    char comment[500];
                    bzero(titlu, 50);
                    bzero(artist, 50);
                    bzero(comment, 500);
                    if (read(client, titlu, 50) <= 0)
                    {
                        perror("[server]Read error AddComment.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    if (read(client, artist, 50) <= 0)
                    {
                        perror("[server]Read error AddComment.\n");
                        close(client);
                        continue; //continui sa ascult
                    }
                    for (int i = 0; i < strlen(titlu); i++)
                        titlu[i] = toupper(titlu[i]);
                    for (int i = 0; i < strlen(artist); i++)
                        artist[i] = toupper(artist[i]);
                    char exista[10];
                    strcpy(res, "NULL");
                    char ss[100];
                    sprintf(ss, "SELECT nr FROM music WHERE UPPER(titlu)='%s' AND UPPER(artist)='%s';", titlu, artist);
                    info(ss);
                    int nr = atoi(res);
                    char s[100];
                    sprintf(s, "SELECT ID FROM users WHERE username='%s' AND password='%s';", usr, pas);
                    info(s);
                    int id = atoi(res);
                    strcpy(exista, res);
                    if (write(client, exista, 10) <= 0)
                    {
                        perror("[server]Write error - AddComment.\n");
                        continue; /* continuam sa ascultam */
                    }
                    if (strcmp(exista, "NULL") != 0)
                    {
                        //citesc comentariul
                        if (read(client, comment, 500) <= 0)
                        {
                            perror("[server]Read error AddComment.\n");
                            close(client);
                            continue; //continui sa ascult
                        }
                        //adaug comentariul
                        sqlite3 *db;
                        char *err_msg = 0;
                        int rc;
                        void *data;
                        /* Open database */
                        rc = sqlite3_open("data", &db);
                        if (rc)
                        {
                            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                            return (0);
                        }
                        char sql[100];
                        sprintf(sql, "INSERT INTO comments (nr, id, com) VALUES ('%d','%d','%s');", nr, id, comment);
                        rc = sqlite3_exec(db, sql, callback1, 0, &err_msg);
                        if (rc != SQLITE_OK)
                        {
                            fprintf(stderr, "SQL error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                            strcpy(msgrasp, "0");
                        }
                        else
                        {
                            fprintf(stdout, "Records created successfully\n");
                            strcpy(msgrasp, "1");
                        }
                        if (write(client, msgrasp, 1) <= 0)
                        {
                            perror("[server]Write error - AddComment.\n");
                            continue; /* continuam sa ascultam */
                        }
                        sqlite3_close(db);
                    }
                }
                else
                    continue;
            }
        }
        return 0;
    }
}