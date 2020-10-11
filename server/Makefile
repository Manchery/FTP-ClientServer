server : server.c connection.h connection.c const.c const.h request.c request.h utils.c utils.h
	gcc -Wall -o server $^ -lpthread

dev : server.c connection.h connection.c const.c const.h request.c request.h utils.c utils.h
	gcc -Wall -g -o server $^ -lpthread

clean : 
	rm ./server