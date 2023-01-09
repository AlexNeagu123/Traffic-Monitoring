#include "utils.h"

struct thread_info {
	int id;
	int client_d;
};

struct command_info {
	char args[MAX_WORDS][MAX_COMMAND_SIZE];
	int args_nr;
};

struct speed_limits {
	int nr;
	char street_names[STR_NR][MAX_STR_NAME];
	double limits[STR_NR];
};

struct speed_limits *global_limits;

struct incidents {
	int nr;
	short type[MAX_INCIDENTS];
	char street[MAX_INCIDENTS][MAX_STR_NAME];
	char by_user[MAX_INCIDENTS][MAX_CRED];
};

struct event_attrs {
	int nr;
	char street[MAX_INCIDENTS][MAX_STR_NAME];
	int freq[MAX_INCIDENTS];
};

struct specific_incident {
	struct event_attrs event[4];
};

struct incidents *incidents_list;

static void *treat_client(void *arg);
void fill_auth_struct(struct auth_creds *new_user, struct command_info *parsed_command);

int configure_server(struct sockaddr_in *server);

short verify_username(char *username);
short verify_credentials(struct user_creds *login_credentials, struct client_data *user_info);

int parse_command(char *command, struct command_info *args, char *err, struct client_data *user_info);

void get_sports_info(char *client_response, char option);
void insert_user_db(struct auth_creds *new_user, char *client_response);

short is_option(char *opt);
short is_date(char *date);
short is_digit(char dig);

void get_weather_info(char *client_response, char *date);
void format_date_news(char *formatted_message, int *cursor, char *curr_date, double morning, double evening, double night, int id);
void get_peco_info(char *client_response, char *station);
void format_peco_info(char *formatted_message, int *cursor, char *peco_name, double gasoline_price, double diesel_price, char *street_name, int peco_id);
void upper(char *str);

void alter_subscribe_data(char *client_response, char *column_name, char *username, int set_to);
void check_speed(char *client_response, double speed, char *street_name, struct speed_limits *limits_loc);
void get_limits_info(struct speed_limits **global_limits);
void insert_incidents_list(short event, char *street_name, char *username);
void get_unread_events(char *client_response, int *incidents_cursor, struct specific_incident *local_incidents);
int find_street_pos(struct specific_incident *local_incidents, short event_type, char *street);
void initialize_incidents(struct specific_incident *local_incidents);
void insert_local_list(struct specific_incident *local_incidents, short event, char *street_name);
void get_event_list(char *client_response, struct specific_incident *local_incidents, short event_type);
void format_event_data(char *formatted_message, int *cursor, char *street_name, int freq,  int id);

pthread_mutex_t incidents_mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
	
	struct sockaddr_in server;
	struct sockaddr_in client;

	pthread_t thread_pool[MAX_CLIENTS];
	int th_cnt = 0;
	int server_d = configure_server(&server);
	
	incidents_list = (struct incidents *) malloc(sizeof(struct incidents));
	incidents_list->nr = 0;

	get_limits_info(&global_limits);

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

static void *treat_client(void *arg)  
{
	struct thread_info th;
	th = *((struct thread_info *) arg);
	pthread_detach(pthread_self());
	
	struct speed_limits limits_loc = *global_limits;
	int incidents_cursor = incidents_list->nr;

	struct specific_incident local_incidents;
	initialize_incidents(&local_incidents);

	int client_d = th.client_d;	
	struct client_data user_info = {"", 0, 0, 0, 0}; 

	printf("[S] Am primit client\n");
	fflush(stdout);

	while(1) 
	{
		char client_command[MAX_COMMAND_SIZE];
		char client_response[CLIENT_RESPONSE];
		
		int code = receive_message(client_d, client_command);
		
		if(code == 0) {
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
			snprintf(client_response, CLIENT_RESPONSE, "Welcome To Faze, %s.", user_info.username);
		}
		
		if(parse_code == 2) {
			// get-sports-info
			char option = '?';
			if(parsed.args_nr == 2) {
				assert(strlen(parsed.args[1]) == 1);
				option = parsed.args[1][0];
			}
			get_sports_info(client_response, option);	
		}

		if(parse_code == 3) {
			//get-weather-info
			char date[DATE_SIZE];
			if(parsed.args_nr == 2) {
				strncpy(date, parsed.args[1], DATE_SIZE);
			}
			else {
				strcpy(date, "any");
			}

			get_weather_info(client_response, date);
			
			int len = strlen(client_response);
			while(len - 1 >= 0 && client_response[len - 1] == '\n') {
				client_response[len - 1] = '\0';
				len--;
			}
		}

		if(parse_code == 4) {
			// get-peco-info 
			char station[MAX_NAME];
			if(parsed.args_nr == 2) {
				strncpy(station, parsed.args[1], MAX_NAME);
			}
			else {
				strncpy(station, "any", MAX_NAME);
			}

			get_peco_info(client_response, station);

			int len = strlen(client_response);
			while (len - 1 >= 0 && client_response[len - 1] == '\n') {
				client_response[len - 1] = '\0';
				len--;
			}
		}

		if(parse_code == 5) {
			// logout
			snprintf(client_response, CLIENT_RESPONSE, "Goodbye, %s!", user_info.username);
			user_info.logged = 0;
			strncpy(user_info.username, "no user", MAX_CRED);
			user_info.peco_sub = -1, user_info.sport_sub = -1, user_info.weather_sub = -1;
		}

		if(parse_code >= 6 && parse_code <= 11) {
			// subscribe channel or unsubscribe channel 
			int set_to = 1;
			if(parse_code >= 9) { 
				set_to = 0;
			}

			char column_name[MAX_NAME];
			
			if(parse_code == 6 || parse_code == 9) {
				user_info.peco_sub = set_to;
				strncpy(column_name, "Peco_Sub", MAX_NAME);
			}
			if(parse_code == 7 || parse_code == 10) {
				user_info.sport_sub = set_to;
				strncpy(column_name, "Sports_Sub", MAX_NAME);
			}
			if(parse_code == 8 || parse_code == 11) {
				user_info.weather_sub = set_to;
				strncpy(column_name, "Weather_Sub", MAX_NAME);
			}

			alter_subscribe_data(client_response, column_name, user_info.username, set_to);
		}

		if(parse_code == 12) {
			// auto-message
			if (!user_info.logged) {
				strncpy(client_response, ShouldLog, CLIENT_RESPONSE);
			}
			else 
			{
				double speed = (double)atof(parsed.args[1]);
				char street_name[MAX_STR_NAME];

				strcpy(street_name, parsed.args[2]);
				for (int i = 3; i < parsed.args_nr; ++i)
				{
					strcat(street_name, " ");
					strcat(street_name, parsed.args[i]);
				}

				// printf("%f %s\n", speed, street_name);
				check_speed(client_response, speed, street_name, &limits_loc);
			}
		}

		if(parse_code == 13) {
			// incident report
			int event = atoi(parsed.args[1]);
			char street_name[MAX_STR_NAME];

			strcpy(street_name, parsed.args[2]);
			for (int i = 3; i < parsed.args_nr; ++i) {
				strcat(street_name, " ");
				strcat(street_name, parsed.args[i]);
			}

			insert_incidents_list(event, street_name, user_info.username);
			
			strncpy(client_response, "The event reported by you was successfully saved.", CLIENT_RESPONSE);
		}

		if(parse_code == 14) {
			// get-events automatic message 
			if(!user_info.logged) {
				strncpy(client_response, ShouldLog, CLIENT_RESPONSE);
			} else {
				get_unread_events(client_response, &incidents_cursor, &local_incidents);
			}
		}

		if(parse_code == 15) {
			get_event_list(client_response, &local_incidents, 3);
		}
		
		if(parse_code == 16) {
			get_event_list(client_response, &local_incidents, 1);
		}

		if(parse_code == 17) {
			get_event_list(client_response, &local_incidents, 0);
		}

		if (parse_code == 18) {
			get_event_list(client_response, &local_incidents, 2);
		}

		if(parse_code == 19) {
			strncpy(client_response, Help, CLIENT_RESPONSE);
		}

		int response_len = strlen(client_response);
		send_message(client_d, client_response, response_len);
		
	}	

	close(client_d);
	return(NULL);
}

int configure_server(struct sockaddr_in *server) 
{
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

int parse_command(char *command, struct command_info *parsed, char *err, struct client_data *user_info) 
{
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
	
	if(!strncmp(command_name, "sign-up", MAX_COMMAND_SIZE)) {
		if (user_info->logged) {
			strncpy(err, AlreadyLogged, MAX_ERR_SIZE);
			return -1;
		}
		if(parsed->args_nr != 8) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!verify_username(parsed->args[3])) {
			strncpy(err, TakenUsername, MAX_ERR_SIZE);
			return -1;
		}
		strcpy(err, Success);
		return 0;
	}

	if(!strncmp(command_name, "login", MAX_COMMAND_SIZE)) {
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
	
	if(!strncmp(command_name, "get-sports-info", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1 && parsed->args_nr != 2) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(parsed->args_nr == 2 && !is_option(parsed->args[1])) {
			strncpy(err, InvalidSportOpt, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->sport_sub) {
			strncpy(err, SportSubscribe, MAX_ERR_SIZE);
			return -1;
		}

		strncpy(err, Success, MAX_ERR_SIZE);
		return 2;	
	}

	if(!strncmp(command_name, "get-weather-info", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1 && parsed->args_nr != 2) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(parsed->args_nr == 2 && !is_date(parsed->args[1])) {
			strncpy(err, NoDate, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->weather_sub) {
			strncpy(err, WeatherSubscribe, MAX_ERR_SIZE);
			return -1;
		}

		strncpy(err, Success, MAX_ERR_SIZE);
		return 3;
	}

	if(!strncmp(command_name, "get-peco-info", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1 && parsed->args_nr != 2) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->peco_sub) {
			strncpy(err, PecoSubscribe, MAX_ERR_SIZE);
			return -1;
		}

		strncpy(err, Success, MAX_ERR_SIZE);
		return 4;
	}

	if(!strncmp(command_name, "logout", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 5;
	}

	if(!strncmp(command_name, "subscribe-peco", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(user_info->peco_sub) {
			strncpy(err, AlreadyPecoSub, MAX_ERR_SIZE);
			return -1;
		}
		return 6;
	}

	if (!strncmp(command_name, "subscribe-sports", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(user_info->sport_sub) {
			strncpy(err, AlreadySportSub, MAX_ERR_SIZE);
			return -1;
		}
		return 7;
	}

	if (!strncmp(command_name, "subscribe-weather", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(user_info->weather_sub) {
			strncpy(err, AlreadyWeatherSub, MAX_ERR_SIZE);
			return -1;
		}
		return 8;
	}

	if(!strncmp(command_name, "unsubscribe-peco", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->peco_sub) {
			strncpy(err, PecoSubscribe, MAX_ERR_SIZE);
			return -1;
		}
		return 9;
	}

	if(!strncmp(command_name, "unsubscribe-sports", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->sport_sub) {
			strncpy(err, SportSubscribe, MAX_ERR_SIZE);
			return -1;
		}
		return 10;
	}

	if(!strncmp(command_name, "unsubscribe-weather", MAX_COMMAND_SIZE)) {
		if(parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if(!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->weather_sub) {
			strncpy(err, WeatherSubscribe, MAX_ERR_SIZE);
			return -1;
		}
		return 11;
	}

	if(!strncmp(command_name, "auto-message", MAX_COMMAND_SIZE)) {
		// this is an automatic message for warnings 100% 
		// TO-DO make a separate check in the user_thread if the user doesnt type auto-message by hand :)))
		assert(parsed->args_nr >= 3);
		return 12;
	}

	if(!strncmp(command_name, "report", MAX_COMMAND_SIZE)) {
		assert(parsed->args_nr >= 3);
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 13;	
	}

	if(!strncmp(command_name, "get-events", MAX_COMMAND_SIZE)) {
		assert(parsed->args_nr == 1);
		return 14;
	}

	if(!strncmp(command_name, "get-police-info", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 15;
	}

	if (!strncmp(command_name, "get-jam-info", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 16;
	}

	if(!strncmp(command_name, "get-accidents-info", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 17;
	}

	if (!strncmp(command_name, "get-repair-info", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		if (!user_info->logged) {
			strncpy(err, ShouldLog, MAX_ERR_SIZE);
			return -1;
		}
		return 18;
	}

	if (!strncmp(command_name, "help", MAX_COMMAND_SIZE)) {
		if (parsed->args_nr != 1) {
			strncpy(err, ValidCommand, MAX_ERR_SIZE);
			return -1;
		}
		return 19;
	}

	strncpy(err, ValidCommand, MAX_ERR_SIZE);
	return -1;
}

short verify_username(char *username) 
{
	sqlite3 *db;
	sqlite3_stmt *stmt;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char get_usernames[MAX_COMMAND_SIZE];
	snprintf(get_usernames, sizeof(get_usernames), "SELECT * FROM Users WHERE Username = \'%s\'", username);

	printf("[S] Database Query On Username Field: %s\n", get_usernames);

	rc = sqlite3_prepare_v2(db, get_usernames, -1, &stmt, NULL);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	rc = sqlite3_step(stmt);
	short return_flag = (rc == SQLITE_DONE);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return return_flag;
}

short verify_credentials(struct user_creds *login_credentials, struct client_data *user_info) 
{
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
		const char *username = (const char *) sqlite3_column_text(stmt, 3);
		short sub_peco = sqlite3_column_int(stmt, 5);
		short sub_weather = sqlite3_column_int(stmt, 6);
		short sub_sports = sqlite3_column_int(stmt, 7);

		strncpy(user_info->username, username, MAX_CRED);
		
		user_info->peco_sub = sub_peco;
		user_info->sport_sub = sub_sports;
		user_info->weather_sub = sub_weather;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return return_flag;
}

void fill_auth_struct(struct auth_creds *new_user, struct command_info *parsed_command) 
{
	// command should have (1+) 7 arguments 
	assert(parsed_command -> args_nr == 8);

	strncpy(new_user->fname, parsed_command->args[1], MAX_NAME);
	strncpy(new_user->lname, parsed_command->args[2], MAX_NAME);
	strncpy(new_user->username, parsed_command->args[3], MAX_CRED);
	strncpy(new_user->password, parsed_command->args[4], MAX_CRED);
	
	new_user->peco_sub = (!strcmp(parsed_command->args[5], "y") ? 1 : 0);
	new_user->sport_sub = (!strcmp(parsed_command->args[6], "y") ? 1 : 0);
	new_user->weather_sub = (!strcmp(parsed_command->args[7], "y") ? 1 : 0);
}

void insert_user_db(struct auth_creds *new_user, char *client_response) 
{
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

short is_option(char *opt) 
{
	return (
		!strcmp(opt, "0")
		|| !strcmp(opt, "1")
		|| !strcmp(opt, "2")
	);
}

void get_sports_info(char *client_response, char option) 
{
	sqlite3 *db;
	sqlite3_stmt *stmt;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char get_sports_query[MAX_COMMAND_SIZE];
	char option_sport[MAX_NAME] = "";

	if(option == '?') {
		snprintf(get_sports_query, MAX_COMMAND_SIZE, "SELECT * FROM Sports ORDER BY RANDOM() LIMIT 1");
	}
	else {
		switch(option) {
			case '0' : strncpy(option_sport, "Football", MAX_NAME); break;
			case '1' : strncpy(option_sport, "Basketball", MAX_NAME); break;
			case '2' : strncpy(option_sport, "Tennis", MAX_NAME); break;
		}

		snprintf(get_sports_query, MAX_COMMAND_SIZE, "SELECT * FROM Sports WHERE Sport_Type = \'%s\'"
				" ORDER BY RANDOM() LIMIT 1", option_sport	
			);
	}

	printf("[S] Database get-sports query: %s\n", get_sports_query);

	rc = sqlite3_prepare_v2(db, get_sports_query, -1, &stmt, 0);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
		const char *data_extracted = (const char *) sqlite3_column_text(stmt, 2);
		strncpy(client_response, data_extracted, CLIENT_RESPONSE);
	}
	else {
		snprintf(client_response, CLIENT_RESPONSE, "There any news about %s in the database:(", option_sport);
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

short is_date(char *date) 
{
	int date_len = strlen(date);
	if(date_len != 10) {
		return 0;
	}
	if(date[2] != '-' || date[5] != '-') {
		return 0;
	}
	for(int i = 0; i < date_len; i++) {
		if(i != 2 && i != 5 && !is_digit(date[i])) {
			return 0;
		}
	}
	return 1;
}

short is_digit(char dig) {
	return (dig >= '0' && dig <= '9');
}

void get_weather_info(char *client_response, char *date) 
{
	sqlite3 *db;
	sqlite3_stmt *stmt;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char get_weather_query[MAX_COMMAND_SIZE];
	
	if(!strcmp(date, "any")) {
		snprintf(get_weather_query, MAX_COMMAND_SIZE, "SELECT * FROM Weather");
	}
	else {
		snprintf(get_weather_query, MAX_COMMAND_SIZE, "SELECT * FROM Weather WHERE Date = \'%s\'", date);
	}

	printf("[S] Database get-weather query: %s\n", get_weather_query);
	
	rc = sqlite3_prepare_v2(db, get_weather_query, -1, &stmt, 0);
	DB_CHECK(!rc, sqlite3_errmsg(db));
	rc = sqlite3_step(stmt);

	if(rc == SQLITE_DONE) {
		strncpy(client_response, "No news available for the corresponding date :(", CLIENT_RESPONSE);
	}
	else {
		int news_id = 1;
		char formatted_message[CLIENT_RESPONSE];
		int cursor = 0;

		while(rc == SQLITE_ROW) {
			char *curr_date = (char *) sqlite3_column_text(stmt, 0);
			double morning = sqlite3_column_double(stmt, 1);
			double evening = sqlite3_column_double(stmt, 2);
			double night = sqlite3_column_double(stmt, 3);

			format_date_news(formatted_message, &cursor, curr_date, morning, evening, night, news_id);
			
			rc = sqlite3_step(stmt);
			news_id++;
		}
		
		DB_CHECK(rc == SQLITE_DONE, sqlite3_errmsg(db));
		strncpy(client_response, formatted_message, CLIENT_RESPONSE);
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

void format_date_news(char *formatted_message, int *cursor, char *curr_date, double morning, double evening, double night, int id) 
{
	char atomic_news[CLIENT_RESPONSE];
	snprintf(atomic_news, CLIENT_RESPONSE, 
			"%d. On date %s, the following temperatures are forecasted:\n%fC in the Morning\n%fC in the Evening\n%fC on Night\n",
			id, curr_date, morning, evening, night
		);
	
	int len_news = strlen(atomic_news);
	strncpy(formatted_message + *cursor, atomic_news, CLIENT_RESPONSE - *cursor);
	*cursor += len_news;
}

void get_peco_info(char *client_response, char *station) 
{
	sqlite3 *db;
	sqlite3_stmt *stmt;

	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char get_peco_query[MAX_COMMAND_SIZE];
	
	if(!strcmp(station, "any")) {
		snprintf(get_peco_query, MAX_COMMAND_SIZE, "SELECT * FROM Pecos NATURAL JOIN Streets");
	}
	else {
		upper(station);
		snprintf(get_peco_query, MAX_COMMAND_SIZE, "SELECT * FROM Pecos NATURAL JOIN Streets WHERE UPPER(Peco_Name) = \'%s\'", station);
	}

	printf("[S] Database get-peco query: %s\n", get_peco_query);

	rc = sqlite3_prepare_v2(db, get_peco_query, -1, &stmt, 0);
	DB_CHECK(!rc, sqlite3_errmsg(db));	
	rc = sqlite3_step(stmt);
	
	int peco_id = 1;
	char formatted_message[CLIENT_RESPONSE];
	int cursor = 0;
	
	if(rc == SQLITE_DONE) {
		strncpy(client_response, "No pecos with this name available nearby :(", CLIENT_RESPONSE);
	}
	else {
		while (rc == SQLITE_ROW) {
			char *peco_name = (char *) sqlite3_column_text(stmt, 1);
			double gasoline_price = sqlite3_column_double(stmt, 3);
			double diesel_price = sqlite3_column_double(stmt, 4);
			char *street_name = (char *) sqlite3_column_text(stmt, 5);

			format_peco_info(formatted_message, &cursor, peco_name, gasoline_price, diesel_price, street_name, peco_id);

			rc = sqlite3_step(stmt);
			peco_id++;
		}

		DB_CHECK(rc == SQLITE_DONE, sqlite3_errmsg(db));
		strncpy(client_response, formatted_message, CLIENT_RESPONSE);
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

void format_peco_info(char *formatted_message, int *cursor, char *peco_name, double gasoline_price, double diesel_price, char *street_name, int peco_id) 
{
	char atomic_info[CLIENT_RESPONSE];
	snprintf(atomic_info, CLIENT_RESPONSE,
			 "%d. There is a %s filling station on the %s street. The prices are as follows:\nGasoline - %fRON\nDiesel - %fRON\n",
			 peco_id, peco_name, street_name, gasoline_price, diesel_price);

	int len_info = strlen(atomic_info);
	strncpy(formatted_message + *cursor, atomic_info, CLIENT_RESPONSE - *cursor);
	*cursor += len_info;
}

void upper(char *str) 
{
	for(int i = 0; *(str + i) != '\0'; i++) {
		if(str[i] >= 'a' && str[i] <= 'z') {
			str[i] -= 'a' - 'A';
		}
	}
}

void alter_subscribe_data(char *client_response, char *column_name, char *username,  int set_to) 
{
	sqlite3 *db;
	
	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char alter_sub_query[MAX_COMMAND_SIZE];
	snprintf(alter_sub_query, MAX_COMMAND_SIZE, "UPDATE Users SET %s = %d WHERE Username = \'%s\'",
			column_name, set_to, username
		);
	
	printf("[S] Altering the Users table: %s\n", alter_sub_query);

	char *err_msg = NULL;
	rc = sqlite3_exec(db, alter_sub_query, NULL, NULL, &err_msg);

	DB_CHECK(!rc, err_msg);

	if(!set_to) {
		strncpy(client_response, UnsubMessage, CLIENT_RESPONSE);
	}
	else {
		strncpy(client_response, SubMessage, CLIENT_RESPONSE);
	}

	sqlite3_close(db);
}

void check_speed(char *client_response, double speed, char *street_name, struct speed_limits *limits_loc) 
{
	int id = 0;
	while(id < limits_loc->nr && strncmp(street_name, limits_loc->street_names[id], MAX_STR_NAME)) {
		id++;
	}

	if(id == limits_loc->nr) {
		// normally shouldn't
		strncpy(client_response, UnknownStreet, CLIENT_RESPONSE);
		return;
	}

	if(limits_loc->limits[id] < speed) {
		int delta = speed - limits_loc->limits[id];
		snprintf(client_response, CLIENT_RESPONSE, "Warning! You are surpassing the speed limit on %s wih %d km/h! " 
		"Your Speed - %d km/h. Legal Speed Limit - %d km/h.", street_name, delta, (int) speed, (int) limits_loc->limits[id]);
	}
	else {
		snprintf(client_response, CLIENT_RESPONSE, "You are driving on %s according to the rules! Your current speed is %d km/h. "
		"The speed limit on this street is %d km/h.", street_name, (int) speed, (int) limits_loc->limits[id]);
	}
}

void get_limits_info(struct speed_limits **global_limits) 
{
	*global_limits = malloc(sizeof(struct speed_limits));
	
	sqlite3 *db;
	sqlite3_stmt *stmt;
	
	int rc = sqlite3_open("Orasul_Chisinau.db", &db);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	char get_speed_limits[MAX_COMMAND_SIZE];
	strncpy(get_speed_limits, "SELECT * FROM Speed_Limits NATURAL JOIN Streets", MAX_COMMAND_SIZE);
	
	printf("[S] Extracting data about speed limits: %s\n", get_speed_limits);
	
	rc = sqlite3_prepare_v2(db, get_speed_limits, -1, &stmt, 0);
	DB_CHECK(!rc, sqlite3_errmsg(db));

	rc = sqlite3_step(stmt);
	while(rc == SQLITE_ROW) {
		int ind = (*global_limits)->nr;
		(*global_limits)->limits[ind] = sqlite3_column_double(stmt, 1);
		strncpy((*global_limits)->street_names[ind], (const char *) sqlite3_column_text(stmt, 2), MAX_STR_NAME);
		(*global_limits)->nr += 1;
		rc = sqlite3_step(stmt);
	}
	DB_CHECK(rc == SQLITE_DONE, sqlite3_errmsg(db));

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

void insert_incidents_list(short event, char *street_name, char *username) 
{
	pthread_mutex_lock(&incidents_mutex);

	printf("[S] Insert %d event on %s street reported by user %s in the incidents_list\n", event, street_name, username);
	
	int ind = incidents_list->nr;
	incidents_list->type[ind] = event;

	strncpy(incidents_list->street[ind], street_name, MAX_STR_NAME);
	strncpy(incidents_list->by_user[ind], username, MAX_CRED);

	incidents_list->nr++;

	pthread_mutex_unlock(&incidents_mutex);
}

void get_unread_events(char *client_response, int *incidents_cursor, struct specific_incident *local_incidents)
{
	pthread_mutex_lock(&incidents_mutex);

	if(*incidents_cursor == incidents_list->nr) {
		strncpy(client_response, NoNotif, CLIENT_RESPONSE);	
	}
	else 
	{
		// printf("%d %d\n", incidents_list->nr, *incidents_cursor);
		int i = *incidents_cursor;

		short event_type = incidents_list->type[i];
		char *street = incidents_list->street[i];

		char *by_user = incidents_list->by_user[i];
		char event_string[MAX_WORDS];
		
		switch (event_type) {
			case 0:
				strncpy(event_string, "was an accident", MAX_WORDS);
				break;
			case 1:
				strncpy(event_string, "is a traffic jam", MAX_WORDS);
				break;
			case 2:
				strncpy(event_string, "is a road reparation", MAX_WORDS);
				break;
			case 3:
				strncpy(event_string, "is a police patrol", MAX_WORDS);
				break;
		}

		snprintf(client_response, CLIENT_RESPONSE, "[New Notification] There %s on the %s street. Reported by %s.",
					event_string, street, by_user
			);

		insert_local_list(local_incidents, event_type, street);
		
		*incidents_cursor = *incidents_cursor + 1;
	}	

	pthread_mutex_unlock(&incidents_mutex);
}

void initialize_incidents(struct specific_incident *local_incidents)
{
	pthread_mutex_lock(&incidents_mutex);
	
	for(int i = 0; i < 4; ++i) {
		local_incidents->event[i].nr = 0;
	}

	for(int i = 0; i < incidents_list->nr; i++) 
	{	
		short event_type = incidents_list->type[i];
		char *street = incidents_list->street[i];

		int street_pos = find_street_pos(local_incidents, event_type, street);
		
		if(street_pos == -1) {
			int pos = local_incidents->event[event_type].nr;
			strncpy(local_incidents->event[event_type].street[pos], street, MAX_STR_NAME);
			local_incidents->event[event_type].freq[pos] = 1;
			local_incidents->event[event_type].nr++;
		}
		else {
			local_incidents->event[event_type].freq[street_pos]++;
		}

	}

	pthread_mutex_unlock(&incidents_mutex);
}

int find_street_pos(struct specific_incident *local_incidents, short event_type, char *street) 
{
	for(int i = 0; i < local_incidents->event[event_type].nr; ++i) {
		if(!strncmp(local_incidents->event[event_type].street[i], street, MAX_STR_NAME)) {
			return i;
		}
	}
	return -1;
}

void insert_local_list(struct specific_incident *local_incidents, short event, char *street_name)
{
	int street_pos = find_street_pos(local_incidents, event, street_name);
	
	if(street_pos == -1) {
		int pos = local_incidents->event[event].nr;
		strncpy(local_incidents->event[event].street[pos], street_name, MAX_STR_NAME);
		local_incidents->event[event].freq[pos] = 1;
		local_incidents->event[event].nr++;
	}
	else {
		local_incidents->event[event].freq[street_pos]++;
	}
}

void get_event_list(char *client_response, struct specific_incident *local_incidents, short event_type)
{
	char event_string[MAX_WORDS];

	switch(event_type) {
		case 0: strncpy(event_string, "accidents", MAX_WORDS); break;
		case 1: strncpy(event_string, "traffic jams", MAX_WORDS); break;
		case 2: strncpy(event_string, "road repairs", MAX_WORDS); break;
		case 3: strncpy(event_string, "police patrols", MAX_WORDS); break;
	}

	int nr = local_incidents->event[event_type].nr;
	
	if(nr == 0) {
		snprintf(client_response, CLIENT_RESPONSE, "There are no %s registered by other users.", event_string);
		return;
	}

	char formatted_message[CLIENT_RESPONSE];

	snprintf(formatted_message, CLIENT_RESPONSE, "The following %s were reported by other users today:\n", event_string);
	int cursor = strlen(formatted_message);

	for(int i = 0; i < nr; ++i) {
		char *street_name = local_incidents->event[event_type].street[i];
		int frequency = local_incidents->event[event_type].freq[i];
		format_event_data(formatted_message, &cursor, street_name, frequency, i);
	}

	int final_len = strlen(formatted_message) - 1;
	
	// redundant newlines
	while(final_len >= 0 && formatted_message[final_len] == '\n') {
		formatted_message[final_len] = '\0';
		final_len--;
	}

	strncpy(client_response, formatted_message, CLIENT_RESPONSE);
}

void format_event_data(char *formatted_message, int *cursor, char *street_name, int freq,  int id)
{
	char atomic_data[CLIENT_RESPONSE];
	snprintf(atomic_data, CLIENT_RESPONSE,
			 "%d. On %s street [Reported by %d users]\n",
			 id, street_name, freq);

	int len_data = strlen(atomic_data);
	strncpy(formatted_message + *cursor, atomic_data, CLIENT_RESPONSE - *cursor);
	*cursor += len_data;
}
