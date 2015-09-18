all: httpdb


httpdb:
	$(CC) -I ../vos -I ../vdb  main.c common.c httpdb_var.c http_ws.c \
	 -lpthread -ldl -lcrypto -lssl

clean:
	rm *.o
	
	
	