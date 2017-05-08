build:
	g++ -Wall server.cpp -o server
	g++ -Wall client.cpp -o client

clean:
	-rm  server client

