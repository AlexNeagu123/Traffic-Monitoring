#ifndef utils
#define utils

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include <arpa/inet.h>
#include <time.h>  
#include "messages.h"

#define PORT 2610
#define ROADS 55
#define MAX_STR_NAME 128
#define MAX_CLIENTS 100
#define MAX_WORDS 100 
#define CTHREADS 5
#define MAX_COMMAND_SIZE 1024
#define CLIENT_RESPONSE 1024
#define MAX_NAME  18
#define SIGN_QUEST 7
#define MAX_ERR_SIZE 512
#define MAX_CRED 255
#define DATE_SIZE 11
#define STR_NR 30
#define MAX_INCIDENTS 1000

#define CHECK(condition, message) \
    if (!(condition))             \
    {                             \
        perror(message);          \
        exit(EXIT_FAILURE);       \
    }

#define DB_CHECK(condition, message)                      \
    if (!(condition))                                     \
    {                                                     \
        fprintf(stderr, "Database Error: %s\n", message); \
        sqlite3_close(db);                                \
        exit(EXIT_FAILURE);                               \
    }

struct user_creds {
    char username[MAX_CRED];
    char password[MAX_CRED];
};

struct auth_creds {
    char fname[MAX_NAME], lname[MAX_NAME];
    char username[MAX_CRED];
    char password[MAX_CRED];
    short peco_sub, sport_sub, weather_sub;
};

struct client_data {
    char username[MAX_CRED];
    short logged, peco_sub, sport_sub, weather_sub;
};

struct node {
    int cross_id;
    int distance;
    char street_name[MAX_STR_NAME];
    struct node *next;
};

struct city_map {
    struct node *adj[ROADS];
};


struct Event {
    int evcode;
    char street_name[MAX_STR_NAME];
};

const char *sign_queries[SIGN_QUEST] = {
        "-> First Name: ", "-> Last Name: ", "-> Username: ", "-> Password: ",
        "-> Subscribe To Peco Channel? [y/n]: ", "-> Subscribe To Sports Channel? [y/n]: ",
        "-> Subscribe To Weather Channel? [y/n]: "
    };

const char *password_query = "-> password: ";
const char *ServerIP = "127.0.0.1";
const char *map_qry_statement = "SELECT * FROM Roads NATURAL JOIN Streets";
const char *data = "Callback Function Call";
char name_repl = '_';
char *errMsg = 0;

int read_from_user(int fd, char *buff) {
    int i = 0;
    while(1) {
        char ch;
        int br = read(fd, &ch, 1);
        CHECK(br != -1, "Error at read()");
        if(br == 0 || ch == '\n') {
            break;
        }
        *(buff+i) = ch;
        i++;
    }
    *(buff+i) = '\0';
    return i;
}

void send_message(int fd, char *buff, int len) {
    CHECK(write(fd, &len, sizeof(int)) != -1, "Error at write()");
    CHECK(write(fd, buff, len) != -1, "Error at write()");
}

int receive_message(int fd, char *buff) {
    int len;
    CHECK(read(fd, &len, sizeof(int)) != -1, "Error at read()");
    CHECK(read(fd, buff, len) != -1, "Error at read()");
    buff[len] = '\0';
    return len;
}

void add_edge(struct node **group, int vec, int dist, char *str_name) {
    struct node *obj = malloc(sizeof(struct node));
    obj->cross_id = vec;
    obj->distance = dist;
    strncpy(obj->street_name, str_name, MAX_STR_NAME);
    obj->next = *group;
    *group = obj;
}

static int get_map(void *data, int argc, char **argv, char **col_name) {
    struct city_map *cmap = *((struct city_map **)data);
    int u = atoi(argv[1]);
    int v = atoi(argv[2]);
    add_edge(&cmap->adj[u], v, atoi(argv[4]), argv[5]);
    add_edge(&cmap->adj[v], u, atoi(argv[4]), argv[5]);
    return 0;
}

void get_map_info(struct city_map **map) {
    sqlite3 *db;
    *map = (struct city_map *) malloc(sizeof(struct city_map));
    int rc = sqlite3_open("Orasul_Chisinau.db", &db);
    DB_CHECK(!rc, sqlite3_errmsg(db));
    rc = sqlite3_exec(db, map_qry_statement, get_map, (void *)map, &errMsg);
    DB_CHECK(!rc, errMsg);
    sqlite3_close(db);
}

#endif 
