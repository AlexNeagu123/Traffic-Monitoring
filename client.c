#include "utils.h"

struct thread_info {
	int id;
	int server_d;
};

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

int main(int argc, char **argv) 
{
	struct sockaddr_in server;
	int server_d = server_info(&server);
	printf("[C] M-am conectat la server\n");

	pthread_t thread_pool[CTHREADS];
	struct thread_info *th[4];
	
	for(int th_cnt = 0; th_cnt < CTHREADS; ++th_cnt) {
		th[th_cnt] = (struct thread_info *) malloc(sizeof(struct thread_info));
		th[th_cnt] -> id = th_cnt;
		th[th_cnt] -> server_d = server_d;
	}

	CHECK(!pthread_create(&thread_pool[0], NULL, &user_thread, th[0]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[1], NULL, &update_thread, th[1]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[2], NULL, &warn_thread, th[2]), "Error at pthread_create()");
	CHECK(!pthread_create(&thread_pool[3], NULL, &notif_thread, th[3]), "Error at pthread_create()");

	void *thread_rv;
	pthread_join(thread_pool[0], &thread_rv);
	int ret_val = (int) thread_rv;

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
	int server_d = th.server_d;
	// printf("Sunt pe threadul: %d\n", th.id);

	return(NULL);
}

static void* user_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);
	int server_d = th.server_d;
	// printf("Sunt pe threadul: %d\n", th.id);

	while(1) {

		printf("-> ");
		fflush(stdout);

		char user_input[MAX_COMMAND_SIZE];
		int len = read_from_user(0, user_input);
		
		if(!strncmp(user_input, "sign-up", len)) {
			signup_prompt(user_input, &len);
		}
		else if(is_login(user_input, len)) {
			password_prompt(user_input, &len);
		}

 		send_message(server_d, user_input, len);

		if (!strncmp(user_input, "quit", 4)) {
			return (void *)0;
		}

		char client_response[CLIENT_RESPONSE];
		receive_message(server_d, client_response);
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
	
	return(NULL);
}

static void* notif_thread(void *arg) 
{
	struct thread_info th;
	th = *((struct thread_info *)arg);
	pthread_detach(pthread_self());
	int server_d = th.server_d;
	//printf("Sunt pe threadul: %d\n", th.id);
	
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