all:
	gcc server.c -lpthread -o server -Wall -l sqlite3 
	gcc client.c -lpthread -o client -Wall -l sqlite3 
clean:
	rm -f server client 
