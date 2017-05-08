#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

using namespace std;

#define MAX_CLIENTS 10
#define BUFLEN 1024

/* Fisiere pentru log */
ofstream fout;
ofstream ferr;

/* Structura de date in care memorez
informatii despre un client conectat */
struct clientsnfo{
	int port;
	char ip[20];
	int socket;
};

/* Afisam eroarea fie in fisierul de stderr, fie pe stderr */
void printErr (int toFile, string buffer){
	if (toFile == 1){
		ferr << buffer;
		ferr.flush();
	}
	else {
		cerr << buffer;
	}
}

/* Afisam eroarea fie in fisierul de stdout, fie pe stdout */
void printOut (int toFile, string buffer){
	if (toFile == 1){
		fout << buffer;
		fout.flush();
	}
	else {
		cout << buffer;
		fflush(stdout);
	}
}

int main (int argc, char** argv){

    ofstream file;

	int sockfd, newsockfd;
	int fdmax;
	unsigned int clilen;

	/* Pt memorarea optiunilor active */
	int download = 0;
	int portno = 0;
	int recursive = 0;
	int everything = 0;
	int toFile = 0;

	char url[BUFLEN];
	char newBuffer[BUFLEN];
	char buffer[BUFLEN];
	char *pointer;

	/* Memoram clientii conectati la server */
	vector <struct clientsnfo> clients;
	struct clientsnfo client;

	/* Verificarea corectitudinii parametrilor programului */
	if (argc < 3){
		cerr << "Utilizare: ./server [-r] [-e] "
		     << "[-o <fisier_log>] -p <port>\n";
		return -1;
	}
    

	for (int i = 1; i < argc; i++){
		
		/* Optiunea recursive este activa */
		if (strcmp (argv[i], "-r") == 0 && recursive == 0){
			recursive = 1;
		}

		/* Optiunea everything este activa */
		else if (strcmp (argv[i], "-e") == 0 && everything == 0){
			everything = 1;
		}

		/* Optiunea -o este activa */
		else if (strcmp (argv[i], "-o") == 0 && toFile == 0){
			toFile = 1;
			i ++;
			if (i > argc - 1){
				cerr << "Utilizare: ./server [-r] [-e] "
				     << "[-o <fisier_log>] -p <port>\n";
				return -1;
			}
			/* Creem fisierele: numefisier[.stdout/.stderr] */
			else {
				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%s.stdout", argv[i]);
				fout.open(buffer);

				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%s.stderr", argv[i]);
				ferr.open(buffer);
			}
		}

		/* Optiunea -p este activa (obligatoriu) -> setam portul */
		else if (strcmp (argv[i], "-p") == 0 && portno == 0){
			i++;
			if (i > argc - 1){
				cerr << "Utilizare: ./server [-r] [-e]"
				     << " [-o <fisier_log>] -p <port>\n";
				return -1;
			}
			else{
				portno = atoi(argv[i]);
			}
		}
		else{
			cerr << "Utilizare: ./server [-r] [-e]"
			     << " [-o <fisier_log>] -p <port>\n";
			return -1;
		}
	}

	/* Daca nu exista portul nu putem executa serverul */
	if (portno == 0){
		cerr << "Utilizare: ./server [-r] [-e] "
		     << "[-o <fisier_log>] -p <port>\n";
		return -1;
	}


		/***** Protocolul TCP *****/

	struct sockaddr_in serv_addr, cli_addr;
	fd_set read_fds;
	fd_set tmp_fds;

	/* Initializarea multimilor de descriptori de fisiere */
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* Obtinere descriptor de fisier - socket folosit 
	pentru conectarea cu clientul */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		printErr(toFile, "Eroare: nu se deschide socketul\n");
		return -1;
	}

	/* Setare informarii pentru socket */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* Asociem pentru socket un port pe masina locala */
	if (bind (sockfd, (struct sockaddr *) &serv_addr,
		      sizeof(struct sockaddr)) < 0){
		printErr(toFile, "Eroare: la asocierea portului\n");
		return -1;
	}

	/* Cautam o noua conexiune client-server */
	if (listen(sockfd, MAX_CLIENTS) < 0){
		printErr(toFile, "Eroare: la ascultarea unei noi conexiuni\n");
		return -1;
	}

	/* Adaugam descriptori de fisiere pentru client si stdin */
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	while (1){
		tmp_fds = read_fds;

		/* Selectam descriptorul ce contine date */
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
			printErr (toFile, "Eroare: la selectarea socketului\n");
			return -1;
		}

		/* Am primit date pe la stdin */
		if (FD_ISSET(0, &tmp_fds)){
			memset(buffer, 0 , BUFLEN);

			fflush(stdout);
			cin.getline (buffer, BUFLEN);

			/* Comanda status - afisam toti clientii din
			lista de clienti conectati */

			if (strcmp (buffer, "status") == 0){
				printOut (toFile, "Status server:\n");

				if (clients.size() == 0){
					printOut (toFile, "Nu exista niciun client conectat.\n");
				}
				/* Afisam informatiile despre clienti */
				else {
					memset (newBuffer, 0, BUFLEN);
					strcpy (newBuffer, "%d. Clientul cu adresa ip %s");
					strcat (newBuffer, " si portul %d ");
					strcat (newBuffer, "este conectat prin socketul %d.\n");

					for (unsigned int i = 0; i < clients.size(); i++){
						memset(buffer, 0, BUFLEN);

						sprintf (buffer, newBuffer, i, clients[i].ip,
						        clients[i].port, clients[i].socket);
					
						printOut(toFile, buffer);
					}
				}
			}

			/* Comanda exit - serverul trimite clientilor mesaj de exit,
			inchide toate legaturile cu acesti si se inchide si el */

			else if (strcmp (buffer, "exit") == 0){

				printOut (toFile, "Exit:\n");

				memset (newBuffer, 0, BUFLEN);
				strcpy (newBuffer, "Conexiunea cu clientul de ");
				strcat (newBuffer, " pe socketul %d");
				strcat (newBuffer, " s-a intrerupt.\n");

				for (unsigned int i = 0; i < clients.size(); i++){
					if (send (clients[i].socket,
						   buffer, strlen(buffer), 0) < 0) {
					   printErr(toFile, "Eroare: scriere date pe socket\n");
					}

					/* folosesc bufferul url pentru a creea mesajul */
					memset(url, 0, BUFLEN);
					sprintf (url, newBuffer, clients[i].socket);

					printOut(toFile, url);

					/* Inchidem conexiune si il scoatem din
					multimea de descriptori */
					close(clients[i].socket); 
					FD_CLR(clients[i].socket, &read_fds);
				}

				/* Stergem clientii salveti si parasim programul */
				clients.erase(clients.begin(), clients.end());
				FD_CLR(0, &read_fds);
				break;
			}

			/* Comanda download */

			else if (strncmp (buffer, "download", 8) == 0){
				memset (url, 0, BUFLEN);
				memset (newBuffer, 0, BUFLEN);

				strcpy (url, buffer + 9);
				sprintf (newBuffer, "Descarcare pagina %s\n", url);
				printOut (toFile, newBuffer);

				/* Despartim comanda in doua cuvinte:
				download si adresa paginii */

				pointer = strtok (buffer, " ");
				
				if (pointer != NULL){
					pointer = strtok (NULL, "");
				
				  if (pointer != NULL){
					memset (url, 0, BUFLEN);
					strcpy (url, pointer);

				    /* Daca avem deja un client conectat
				    la server - ii trimitem comanda download */
					download = 1;
					if (clients.size() > 0){
				      for (unsigned int k = 0; k < clients.size(); k++){

				      	int s = send(clients[0].socket,
								      url, strlen(url), 0);

						if ( s < 0) {
							printErr(toFile,
								"Eroare: scriere date pe socket\n");
						}
						else{
							download = 0;
							break;
						}
					  }
					}
					else {
						printErr(toFile, "Nu exista clienti conectati. Se ");
						printErr(toFile, "asteapta conectarea unuia..\n");
					}
				  }
				  else {
					printErr(toFile, "Eroare: link incorect\n");
				  }					
				}
			}
			/* Serverul nu cunoaste aceasta comanda */
			else {
				printErr(toFile, "Eroare: comanda necunoscuta!\n");
			}
		}

		/* Am primit date de la unul din clienti */
		for (int i = 1; i <= fdmax; i++){
			if (FD_ISSET (i, &tmp_fds)){
				
				if (i == sockfd) {
					/* A venit ceva pe socketul inactiv(cel cu listen), deci
					o noua conexiune. Actiunea serverului: accept() */
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
										 &clilen);

					if (newsockfd == -1) {
						printErr(toFile, "Eroare: acceptare socket\n");
					} 
					else {
						/* Adaug noul socket intors de accept()
						la multimea descriptorilor de citire */
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					memset (newBuffer, 0, BUFLEN);
					strcpy (newBuffer, "O noua conexiune de la %s");
					strcat (newBuffer, ", portul %d");
					strcat (newBuffer, ", pe socketul %d.\n");

					memset(buffer, 0, BUFLEN);
					sprintf (buffer, newBuffer, inet_ntoa(cli_addr.sin_addr),
					         ntohs(cli_addr.sin_port), newsockfd);

					printOut(toFile, buffer);
				
					/* Memoram noul client in lista de clienti activi */
					client.port = ntohs(cli_addr.sin_port);
					client.socket = newsockfd;
					memset (client.ip, 0, 20);
					strcpy(client.ip, inet_ntoa(cli_addr.sin_addr));

					clients.push_back(client);

					/* Trimit mesaj de confirmare a conexiunii */
					memset (buffer, 0, BUFLEN);
					strcpy (buffer, "Clientul s-a conectat cu succes.");

					/* Comanda de download a fost data inainte de a
					se conecta cineva, deci va fi trimisa primului
					client ce se conecteaza la server */
					if (download == 1){
						strcat (buffer, url);
						download = 0;
					}
					if (send(newsockfd, buffer, strlen(buffer), 0) < 0) {
						printErr(toFile, "Eroare: scriere date pe socket\n");
					}
				}

				/* Primesc date de pe un socket asociat deja unui client */
				else {
					memset(buffer, 0, BUFLEN);
					int r = recv(i, buffer, sizeof(buffer), 0);

					if (r <= 0) {
						if (r == 0) {
							memset (newBuffer, 0, BUFLEN);
							strcpy (newBuffer, "Conexiunea cu clientul ");
							strcat (newBuffer, "de pe socketul %d");
							strcat (newBuffer, " s-a intrerupt.\n");

							memset(buffer, 0, BUFLEN);
							sprintf (buffer, newBuffer, i);

							printOut(toFile, buffer);
						}
						else {
							printErr (toFile,
							    "Eroare: citire date de pe socket\n");
						}

						/* Stergem clientul din lista de clienti activi,
						inchidem conexiunea si il scoatem din
						multimea de descriptori de fisiere */

						for (unsigned int k = 0; k < clients.size(); k++){
							if(clients[k].socket == i){
								clients.erase(clients.begin() + k);
								break;
							}
						}
						close(i); 
						FD_CLR(i, &read_fds);
					}
					
					/* Am primit date de la un client */
					else {

					if (strncmp (buffer, "start-download", 14) == 0){

						/* Afisam inceperea descarcarii */
						strcpy (buffer, buffer + 15);
						
						memset(newBuffer, 0, BUFLEN);
						sprintf(newBuffer,
						  "Pagina http://%s este in curs de descarcare..\n",
						  buffer);

						printOut (toFile, newBuffer);

						memset (newBuffer, 0, BUFLEN);
						memset (url, 0, BUFLEN);
						strcpy (url, buffer);

						/* Extragem calea - vom cauta ultima aparitie a
						caracterului '/', ce este pana la el reprezinta
						ierarhia de directoare */

						pointer = strrchr(buffer, '/');
						strncpy (newBuffer, buffer, (int) (pointer-buffer));

						/* Creem ierarhia de directoare */
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "mkdir -p %s", newBuffer);
						system(buffer);

						/* Verificam daca la finalul adresei url
						exista deja extensia .html pentru fisier */
						memset (buffer, 0, BUFLEN);
						strcpy (buffer, url + (strlen (url) - 5));
						if (strcmp (buffer, ".html")){
							strcat (url, ".html");
						}

						/* Deschidere fisier pentru descarcare -
						trebuie sa ii dam toata calea */
						file.open(url);
						
					}
					/* Descarcarea s-a terminat - afisam acest
					lucru si inchidem fisierul descarcat */
					if (strncmp (buffer, "stop-download", 13) == 0){

						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "Pagina a fost descarcata: %s\n",
								url);

						printOut (toFile, buffer);
						file.close();
					}

					/* Primim date cu continutul fisierului de descarcat */
					else {
						file << buffer;
						file.flush();
					}
					}
				} 
			}
		}
	}

	close (sockfd);

	/* Inchidem fisierele daca au fost folosite */
	if (toFile == 1){
		fout.close();
		ferr.close();
	}

	return 0;
}
