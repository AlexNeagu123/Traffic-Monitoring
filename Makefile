build:
	rm -rf ./bin/server || echo -n '' 
	rm -rf ./bin/client || echo -n ''
	
	gcc ./src/server.c -lpthread -o ./bin/server -Wall -l sqlite3 
	gcc ./src/client.c -lpthread -o ./bin/client -Wall -l sqlite3