#include "utils.h"

struct thread_info {
	int id;
	int client_d;
};

struct command_info {
	char args[MAX_WORDS][MAX_COMMAND_SIZE];
	int args_nr;
};

static void *treat_client(void *arg);
void fill_auth_struct(struct auth_creds *new_user, struct command_info *parsed_command);

int configure_server(struct sockaddr_in *server);

short verify_username(char *username);
short verify_credentials(struct user_creds *login_credentials, struct client_data *user_info);

static int search_username(void *data, int argc, char **argv, char **col_name);
int parse_command(char *command, struct command_info *args, char *err, struct client_data *user_info);

void insert_user_db(struct auth_creds *new_user, char *client_response);

int main() {
	
	struct sockaddr_in server;
	struct sockaddr_in client;

	pthread_t thread_pool[MAX_CLIENTS];
	int th_cnt = 0;
	int server_d = configure_server(&server);
	
	get_map_info();
	
	CHECK(listen(server_d, 10) != -1, ListenErr);

	while(1) {
		struct thread_info *th;
		socklen_t len = sizeof(client);

		printf("[S] Waiting at port %d ...\n", PORT);
		fflush(stdout);
		
		int client_d = accept(server_d, (struct sockaddr *) &client, &len);
		CHECK(client_d != -1, AcceptErr);
		th = (struct thread_info *) malloc(sizeof(struct thread_info));
		th->id = th_cnt++;
		th->client_d = client_d;
		pthread_create(&thread_pool[th->id], NULL, &treat_client, th);
	}

	return 0;

}

static void *treat_client(void *arg) {
	struct thread_info th;
	th = *((struct thread_info *) arg);
	pthread_detach(pthread_self());
	
	int client_d = th.client_d;	
	struct client_data user_info = {"", 0, 0, 0, 0}; 

	printf("[S] Am primit client\n");
	fflush(stdout);

	while(1) {
		char client_command[MAX_COMMAND_SIZE];
		char client_response[CLIENT_RESPONSE];
		int len = receive_message(client_d, client_command);

		if(!strncmp(client_command, "quit", 4)) {
			break;
		}

		struct command_info parsed;

		char ErrorMsg[MAX_ERR_SIZE];
		int parse_code = parse_command(client_command, &parsed, ErrorMsg, &user_info);

		if(parse_code == -1) {
			strncpy(client_response, ErrorMsg, MAX_ERR_SIZE);
		}

		if(parse_code == 0) {
			struct auth_creds new_user;
			fill_auth_struct(&new_user, &parsed);
			insert_user_db(&new_user, client_response);
		}

		if(parse_code == 1) {
			user_info.logged = 1;
			snprintf(client_response, CLIENT_RESPONSE, "Welcome To Faze, %s!", user_info.username);
		}
				
		int response_len = strlen(client_response);
		send_message(client_d, client_response, response_len);
	}	

	close(client_d);
	return(NULL);
}

int configure_server(struct sockaddr_in *server) {
	int server_d = socket(AF_INET, SOCK_STREAM, 0);
	CHECK(server_d != -1, SocketErr);
	int reuse = 1;

	setsockopt(server_d, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	bzero(server, sizeof(*server));

	server->sin_family = AF_INET;
	server->sin_addr.s_addr = htonl(INADDR_ANY);
	server->sin_port = htons(PORT);

	int bind_ret = bind(server_d, (struct sockaddr *)server, sizeof(struct sockaddr));
	CHECK(bind_ret != -1, BindErr);

	return server_d;
}

int parse_command(char *command, struct command_info *parsed, char *err, struct client_data *user_info) {
	int len = strlen(command);
	char cpy[len + 1];

	parsed->args_nr = 0;
	
	strcpy(cpy, command);
	char *token = strtok(cpy, " ");

	while(token != NULL) {
		int pos = parsed->args_nr++;
		strncpy(parsed->args[pos], token, MAX_COMMAND_SIZE);
		token = strtok(NULL, " ");
	}

	if(!parsed->args_nr) {
		strncpy(err, ValidCommand, MAX_ERR_SIZE);
		return -1;
	}

	char *command_name = parsed->args[0];
	
	if(!strncmp(command_name, "sign-up", 7)) {
		if(!verify_username(parsed->args[3])) {
			strncpy(err, TakenUsername, MAX_ERR_SIZE);
			return -1;
		}
		strcpy(err, Success);
		return 0;
	}

	if(!strncmp(command_name, "login", 5)) {
		if(user_info->logged) {
			strncpy(err, AlreadyLogged, MAX_ERR_SIZE);
			return -1;
		} 
		if (parsed->args_nr != 3) {
			strncpy(err, FieldNumber, MAX_ERR_SIZE);
			return -1;
		}

		struct user_creds credentials;

		strncpy(credentials.username, parsed->args[1], MAX_CRED);
		strncpy(credentials.password, parsed->args[2], MAX_CRED);

		if(!verify_credentials(&credentials, user_info)) {
			strncpy(err, InvalidCredentials, MAX_ERR_SIZE);
			return -1;
		}

		strncpy(err, Success, MAX_ERR_SIZE);
		return 1;
	}

	strncpy(err, ValidCommand, MAX_ERR_SIZE);
	return -1;
}

short verify_username(char *username) {
	sqlite3 *db;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));
	
	sqlite3_stmt *stmt;

	char get_usernames[MAX_COMMAND_SIZE];
	snprintf(get_usernames, sizeof(get_usernames), "SELECT * FROM Users WHERE Username = \'%s\'\0", username);

	printf("[S] Database Query On Username Field: %s\n", get_usernames);

	rc = sqlite3_prepare_v2(db, get_usernames, -1, &stmt, NULL);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	rc = sqlite3_step(stmt);
	short return_flag = (rc == SQLITE_DONE);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return return_flag;
}

short verify_credentials(struct user_creds *login_credentials, struct client_data *user_info) {
	sqlite3 *db;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	sqlite3_stmt *stmt;

	char get_credentials[MAX_COMMAND_SIZE];
	snprintf(get_credentials, sizeof(get_credentials), 
				"SELECT * FROM Users WHERE Username = \'%s\' AND Password = \'%s\'",  
				login_credentials->username, login_credentials->password
			);

	printf("[S] Database Query On (Username, Password) Tuple: (%s, %s)\n", login_credentials->username, login_credentials->password);
	printf("[S] %s\n", get_credentials);
	
	rc = sqlite3_prepare_v2(db, get_credentials, -1, &stmt, NULL);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	rc = sqlite3_step(stmt);
	short return_flag = (rc == SQLITE_ROW);
	
	if(return_flag) {
		const char *username = sqlite3_column_text(stmt, 3);
		short sub_peco = sqlite3_column_int(stmt, 5);
		short sub_sports = sqlite3_column_int(stmt, 6);
		short sub_weather = sqlite3_column_int(stmt, 7);
		
		strncpy(user_info->username, username, MAX_CRED);
		user_info->peco_sub = sub_peco;
		user_info->sport_sub = sub_sports;
		user_info->weather_sub = sub_weather;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	
	return return_flag;
}

void fill_auth_struct(struct auth_creds *new_user, struct command_info *parsed_command) {
	// command should have (1+) 7 arguments 
	assert(parsed_command -> args_nr == 8);

	strncpy(new_user->fname, parsed_command->args[1], MAX_NAME);
	strncpy(new_user->lname, parsed_command->args[2], MAX_NAME);
	strncpy(new_user->username, parsed_command->args[3], MAX_CRED);
	strncpy(new_user->password, parsed_command->args[4], MAX_CRED);
	
	new_user->peco_sub = (!strcmp(parsed_command->args[5], "y") ? 1 : 0);
	new_user->sport_sub = (!strcmp(parsed_command->args[5], "y") ? 1 : 0);
	new_user->weather_sub = (!strcmp(parsed_command->args[5], "y") ? 1 : 0);
}

void insert_user_db(struct auth_creds *new_user, char *client_response) {
	sqlite3 *db;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char insert_user_query[MAX_COMMAND_SIZE];
	
	snprintf(insert_user_query, MAX_COMMAND_SIZE, 
				"INSERT INTO Users (Fname, Lname, Username, Password, Peco_Sub, Weather_Sub, Sports_Sub) "
				"VALUES(\'%s\', \'%s\', \'%s\', \'%s\', %d, %d, %d)", new_user->fname, new_user->lname, new_user->username, 
				new_user->password, new_user->peco_sub, new_user->sport_sub, new_user->weather_sub
			);
	
	printf("Database insertion command: %s\n", insert_user_query);
	
	rc = sqlite3_exec(db, insert_user_query, NULL, NULL, NULL);

	if(rc != SQLITE_OK) {
		strncpy(client_response, sqlite3_errmsg(db), CLIENT_RESPONSE);
	}
	else { 
		strncpy(client_response, AuthSuccess, CLIENT_RESPONSE);
	}

	sqlite3_close(db);
}