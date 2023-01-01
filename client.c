#include "utils.h"

struct GPS {
	double speed, distance;
	char street_name[MAX_STR_NAME];
	int prd_cross, nxt_cross;
};

struct thread_info {
	int id;
	int server_d;
	struct GPS *client_gps;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int server_info(struct sockaddr_in *server);
void signup_prompt(char *buff, int *len);
void password_prompt(char *buff, int *len);
short is_login(char *buff, int len);

static void* update_thread(void *arg);
static void* user_thread(void *arg);
static void* warn_thread(void *arg);
static void* notif_thread(void *arg);
short valid_username(char *usr, int len);
short valid_password(char *psw, int len);
short valid_sub(char *sub, int len);
void convert_name(char *name, int len);
void ask_receive_from_server(int server_d, char *command, int comm_len, char *client_response);

short is_report(char *buff, int len);
short valid_report(char *buff, int len);
void format_report(char *user_input, int *len, char *street_name);
short check_reserved(char *user_input, int len);

int main(int argc, char **argv) 
{
	struct sockaddr_in server;
	int server_d = server_info(&server);
	printf("[C] M-am conectat la server\n");

	pthread_t thread_pool[CTHREADS];
	struct thread_info *th[4];

	struct GPS *driver_gps = (struct GPS *) malloc(sizeof(struct GPS));
	
	for(int th_cnt = 0; th_cnt < CTHREADS; ++th_cnt) {
		th[th_cnt] = (struct thread_info *) malloc(sizeof(struct thread_info));
		th[th_cnt]->id = th_cnt;
		th[th_cnt]->server_d = server_d;
		th[th_cnt]->client_gps = driver_gps;
	}

	CHECK(!pthread_create(&thread_pool[0], NULL, &user_thread, th[0]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[1], NULL, &update_thread, th[1]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[2], NULL, &warn_thread, th[2]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[3], NULL, &notif_thread, th[3]), "Error at pthread_create()");

	void *thread_rv;
	pthread_join(thread_pool[0], &thread_rv);
	int ret_val =  *((int*)thread_rv);

	if(ret_val == 0) {
		printf("You have exited the application. Goodbye.\n");
	}
	else {
		printf("%s\n", AppCrash);
	}

	return 0;
}

int server_info(struct sockaddr_in *server) 
{
	int server_d = socket(AF_INET, SOCK_STREAM, 0);
	CHECK(server_d != -1, SocketErr);

	server->sin_family = AF_INET;
	server->sin_addr.s_addr = inet_addr(ServerIP);
	server->sin_port = htons(PORT);
	
	int conn_ret = connect(server_d, (struct sockaddr *)server, sizeof(struct sockaddr));
	CHECK(conn_ret != -1, ConnectErr);
	return server_d;
}

static void* update_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);
	pthread_detach(pthread_self());
	
	srand(time(0));

	struct GPS *gps = th.client_gps;
	struct city_map *map = NULL;
	struct node *j = NULL;

	get_map_info(&map);

	int neighbours[ROADS];
	memset(neighbours, 0, sizeof(neighbours));

	// counting the number of neighbouring intersection for each intersection
	for(int i = 0; i < ROADS; ++i) {
		if(map->adj[i]) {
			j = map->adj[i];
			while(j != NULL) {
				neighbours[i]++;
				j = j->next;
			}
		}
	}

	int init_cross = rand() % ROADS;
	while(map->adj[init_cross] == NULL) {
		init_cross = rand() % ROADS;
	}

	int pos = rand() % neighbours[init_cross];
	j = map->adj[init_cross];

	while(pos && j->next != NULL) {
		j = j->next;
		pos--;
	}

	gps->prd_cross = init_cross;
	gps->nxt_cross = j->cross_id;
	gps->distance = j->distance;
	strncpy(gps->street_name, j->street_name, MAX_STR_NAME);
	gps->speed = 20 + rand() % 31;

	// printf("%d %d %f %f\n", gps->prd_cross, gps->nxt_cross, gps->distance, gps->speed);
	
	while(1) {
		// updates after every second
		sleep(1);
		// speed * 1000 / 3600 * 3 seconds (1 second in reality = 3 seconds in the simulation)
		double travelled_distance = gps->speed * (1000.0 / 3600.0) * 3;
		gps->distance -= travelled_distance;
		
		if(gps->distance < 0) {
			// randomly choose the next intersection to go 
			int nxt_pos = rand() % neighbours[gps->nxt_cross];
			j = map->adj[gps->nxt_cross];

			while (nxt_pos && j->next != NULL) {
				j = j->next;
				nxt_pos--;
			}
			// update everything in the gps structure 
			int aux = gps->nxt_cross;
			gps->nxt_cross = j->cross_id;
			gps->prd_cross = aux;
			gps->distance = j->distance;
			strncpy(gps->street_name, j->street_name, MAX_STR_NAME);
		}
		
		int decision = rand() % 3;
		// (0 -> lower the speed with 1 km/h) (1 -> maintain the current speed) (2 -> increase the speed with 1 km/h)
		if(decision == 0 && gps->speed > 10) {
			gps->speed--;
		}
		else if(decision == 1) {
			gps->speed++;
		}
	}

	return(NULL);
}

static void* user_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);

	struct GPS *gps = th.client_gps;
	int server_d = th.server_d;
	
	// printf("Sunt pe threadul: %d\n", th.id);

	while(1) {

		printf("-> ");
		fflush(stdout);

		char user_input[MAX_COMMAND_SIZE];
		int len = read_from_user(0, user_input);
		
		if(!len) {
			continue;
		}

		if (!strncmp(user_input, "quit", 4)) {
			exit(0);
		}

		char client_response[CLIENT_RESPONSE];

		if (check_reserved(user_input, len)) {
			strncpy(client_response, ValidCommand, CLIENT_RESPONSE);
			goto print_msg;
		}

		if (!strcmp(user_input, "sign-up")) {
			signup_prompt(user_input, &len);
		}
		
		if (is_login(user_input, len)) {
			password_prompt(user_input, &len);
		}

		if (is_report(user_input, len)) {
			if (!valid_report(user_input, len)) {
				strncpy(client_response, ValidCommand, CLIENT_RESPONSE);
				goto print_msg;
			}
			else {
				format_report(user_input, &len, gps->street_name);
			}
		}

		ask_receive_from_server(server_d, user_input, len, client_response);
		
		print_msg:
		printf("Serverul mi-a raspuns cu %s\n", client_response);
	
	}

	return (void *)1;
}

static void* warn_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);
	pthread_detach(pthread_self());
	int server_d = th.server_d;
	//printf("Sunt pe threadul: %d\n", th.id);
	struct GPS *gps = th.client_gps;
	
	while(1) 
	{
		sleep(1);

		char auto_command[MAX_COMMAND_SIZE];
		snprintf(auto_command, MAX_COMMAND_SIZE, "auto-message %f %s", gps->speed, gps->street_name);		
		int comm_len = strlen(auto_command);

		char client_response[CLIENT_RESPONSE];
		ask_receive_from_server(server_d, auto_command, comm_len, client_response);
		// if (strncmp(client_response, NoNotif, CLIENT_RESPONSE)) {
		// 	printf("Serverul a raspuns cu %s\n", client_response);
		// }
	}

	return(NULL);
}

static void* notif_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);
	pthread_detach(pthread_self());
	int server_d = th.server_d;
	//printf("Sunt pe threadul: %d\n", th.id);
	while(1) {
		sleep(5);

		char get_incidents_command[MAX_COMMAND_SIZE];
		strncpy(get_incidents_command, "get-events", MAX_COMMAND_SIZE);
		int comm_len = strlen(get_incidents_command);

		char client_response[CLIENT_RESPONSE];
		ask_receive_from_server(server_d, get_incidents_command, comm_len, client_response);

		if(strncmp(client_response, NoNotif, CLIENT_RESPONSE)) {
			printf("Serverul a raspuns cu %s\n", client_response);
		}
	}

	return(NULL);
}

void signup_prompt(char *buff, int *buff_len) 
{
	char response[MAX_NAME];

	for (int i = 0; i < SIGN_QUEST; ++i) {

		printf("%s", sign_queries[i]);
		fflush(stdout);
		strcat(buff, " ");

		int response_len = read_from_user(0, response);
		
		if(!i || i == 1) {
			convert_name(response, response_len);
		}
		while(i == 2 && !valid_username(response, response_len)) {
			printf("%s", UsernameErr);
			printf("%s", sign_queries[i]);
			fflush(stdout);
			response_len = read_from_user(0, response);
		}
		while(i == 3 && !valid_password(response, response_len)) {
			printf("%s", PasswordErr);
			printf("%s", sign_queries[i]);
			fflush(stdout);
			response_len = read_from_user(0, response);
		}
		while((i == 4 || i == 5 || i == 6) && !valid_sub(response, response_len)) {
			printf("%s", SubErr);
			printf("%s", sign_queries[i]);
			fflush(stdout);
			response_len = read_from_user(0, response);
		}
		strcat(buff, response);
	}

	*buff_len = strlen(buff);
}

void password_prompt(char *buff, int *buff_len) 
{
	char response[MAX_NAME];
	printf("%s", password_query);
	fflush(stdout);

	strcat(buff, " ");
	read_from_user(0, response);
	
	strcat(buff, response);
	*buff_len = strlen(buff);
}

short is_login(char *buff, int len) 
{
	char cpy[len + 1];
	strncpy(cpy, buff, len);
	char *token = strtok(cpy, " ");
	return !strncmp(token, "login", 5) ? 1 : 0;  
}

short valid_username(char *usr, int len) {
	return (!strchr(usr, ' ') && len >= 5);
}

short valid_password(char *psw, int len) {
	return (!strchr(psw, ' ') && len >= 6);
}

short valid_sub(char *sub, int len) {
	return (!strcmp(sub, "y") || !strcmp(sub, "n"));
}

void convert_name(char *name, int len) 
{
	for(int i = 0; i < len; ++i) {
		if(*(name + i) == ' ') {
			*(name + i) = name_repl;
		}
	}
}

void ask_receive_from_server(int server_d, char *command, int comm_len, char *client_response)
{
	pthread_mutex_lock(&mutex);
	send_message(server_d, command, comm_len);

	receive_message(server_d, client_response);
	pthread_mutex_unlock(&mutex);
}

short is_report(char *buff, int len)
{
	char cpy[len + 1];
	strncpy(cpy, buff, len + 1);
	char *token = strtok(cpy, " ");
	return !strcmp(token, "report") ? 1 : 0;
}

short valid_report(char *buff, int len) 
{
	char cpy[MAX_COMMAND_SIZE];
	strncpy(cpy, buff, MAX_COMMAND_SIZE);

	char *token = strtok(cpy, " ");
	
	char last_param[MAX_COMMAND_SIZE];
	int nr_tokens = 0;

	while (token != NULL) {
		strncpy(last_param, token, MAX_COMMAND_SIZE);
		token = strtok(NULL, " ");
		nr_tokens++;
	}

	if(nr_tokens != 2) {
		return 0;
	}

	if(strlen(last_param) != 1) {
		return 0;
	}

	char option = last_param[0];
	return (option >= '0' && option <= '3');
}

void format_report(char *user_input, int *len, char *street_name) {
	strcat(user_input, " ");
	strcat(user_input, street_name);
	*len = strlen(user_input);
}

short check_reserved(char *user_input, int len) {
	char cpy[len + 1];
	strncpy(cpy, user_input, len + 1);
	char *token = strtok(cpy, " ");
	return (!strcmp(token, "auto-message") || !strcmp(token, "get-events"));
}