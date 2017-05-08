#include <iostream>
#include <fstream>
#include <cstring>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

using namespace std;

#define BUFLEN 1024

/* Fisiere pentru log */
ofstream fout;
ofstream ferr;

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

	/* Pt memorarea optiunilor active */
	int toFile = 0;
	int portno = 0;
	int hasIp = 0;
	int pid;

	int sockfd, sockhttp;
	int numfds = 11;
	

	char buffer[BUFLEN];
	char url[BUFLEN];
	char newBuffer[BUFLEN];

	char ip_server[20];
	char *pointer;

	struct sockaddr_in serv_addr;
	fd_set read_fds;
	fd_set tmp_fds;

	/* Verificarea corectitudinii parametrilor programului */

	if (argc < 5){
		cerr << "Utilizare: ./client [-o <fisier_log>]"
		     <<" -a <adresa ip server> -p <port>\n";
		return -1;
	}

	for (int i = 1; i < argc; i++){

		/* Optiunea -o este activa */
		if (strcmp (argv[i], "-o") == 0 && toFile == 0){
			toFile = 1;
			i ++;

			if (i > argc - 1){
				cerr << "Utilizare: ./client [-o <fisier_log>]"
				     << " -a <adresa ip server> -p <port>\n";
				return -1;
			}
			/* Creem fisierele: nume-pid[.stdout/.stderr] */
			else {
				pid = getpid();
				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%s-%d.stdout", argv[i], (int) pid);
				fout.open(buffer);

				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%s-%d.stderr", argv[i], (int) pid);
				ferr.open(buffer);
			}
		}

		/* Optiunea -a este activa (obligatoriu) -> setam adresa ip */
		else if (strcmp (argv[i], "-a") == 0 && hasIp == 0){
			i++;
			if (i > argc - 1){
				cerr << "Utilizare: ./client [-o <fisier_log>] "
				     << "-a <adresa ip server> -p <port>\n";
				return -1;
			}
			else{
				memset (ip_server, 0, 20);
				strcpy(ip_server, argv[i]);
				hasIp = 1;
			}
		}

		/* Optiunea -p este activa (obligatoriu) -> setam portul */
		else if (strcmp (argv[i], "-p") == 0 && portno == 0){
			i++;
			if (i > argc - 1){
				cerr << "Utilizare: ./client [-o <fisier_log>] "
				     << "-a <adresa ip server> -p <port>\n";
				return -1;
			}
			else{
				portno = atoi(argv[i]);
			}
		}
		else {			
			cerr << "Utilizare: ./client [-o <fisier_log>] "
			     << "-a <adresa ip server> -p <port>\n";
			return -1;
		}
	}

	/* Daca nu a fost dat portul sau adresa ip */
	if (hasIp == 0 || portno == 0) {			
		cerr << "Utilizare: ./client [-o <fisier_log>] "
		     << "-a <adresa ip server> -p <port>\n";
		return -1;
	}

		/***** Protocolul TCP *****/

	/* Initializarea multimilor de descriptori de fisiere */

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* Obtinere descriptor de fisier - socket folosit 
	pentru conectarea cu serverul */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		printErr(toFile, "Eroare: nu se deschide socketul\n");
		return -1;
	}
	 
	/* Setare informarii pentru socket */ 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	inet_aton(ip_server, &serv_addr.sin_addr);
	
	/* Stabilirea conexiunii */	
	if (connect(sockfd, (struct sockaddr*) &serv_addr,
                sizeof(serv_addr)) < 0){
		printErr(toFile, "Eroare: la conectarea cu serverul\n");		
		return -1;
	}

	/* Adaugam descriptori de fisiere pentru server */
	FD_SET(sockfd, &read_fds);


	while(1){
		tmp_fds = read_fds;

		/* Selectam descriptorul ce contine date */
		if (select(numfds + 1, &tmp_fds, NULL, NULL, NULL) == -1){
			printErr (toFile, "Eroare: la selectarea socketului\n");
		}

		/* Am primit date pe socketul serverului */
		if (FD_ISSET (sockfd, &tmp_fds)){
			memset(buffer, 0 , BUFLEN);

			/* Citesc date de pe socket */
			if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
				printErr (toFile, "Eroare: citire date de pe socket\n");
			}

			/* Verificam daca clientul s-a conectat cu succes */
			if (strncmp (buffer,
				"Clientul s-a conectat cu succes.", 32) == 0){

				/* In buffer este posibil sa existe dupa sirul
				de mai sus si o comanda de download */
				memset (newBuffer, 0 ,BUFLEN);
  				strncpy (newBuffer, buffer, 32);
  				strcat (newBuffer, "\n");
  				printOut (toFile, newBuffer);

  				/* Serverul cere si descarcarea unei paginii
  				la conectarea clientului */
  				if (strlen (buffer) > 32){
  					strcpy (buffer, buffer + 32);
  				}
			}

			    /****  EXIT *****/
			/* Serverul urmeaza sa se inchida sau s-a inchis deja */

			if (strcmp (buffer, "exit") == 0 ||
				strcmp (buffer, "") == 0){

				memset(buffer, 0, BUFLEN);
				strcpy (buffer, "Serverul s-a inchis!\n");
				printOut (toFile, buffer);

				/* Inchidem conexiunea si terminam executia programului */
				FD_CLR(sockfd, &read_fds);
				close (sockfd);
				break;
			}

			/* Serverul cere descarcarea unei pagini */
			else if (strncmp (buffer, "http://", 7) == 0){

				/* Afisam comanda serverului*/
				memset (url, 0, BUFLEN);
				strcpy (url, "Serverul a cerut descarcarea paginii ");
				strcat (url, buffer);
				strcat (url, "\n");
				printOut (toFile, url);

				strcpy (buffer, buffer + 7);
				memset (newBuffer, 0 ,BUFLEN);
				memset (url, 0, BUFLEN);

				/* Extragem din adresa primita hostul si calea catre pagina */

				pointer = strchr (buffer, '/');
  				strncpy (newBuffer, buffer, (int)(pointer - buffer));
				strcpy (url, buffer + (pointer - buffer));
  				
  				/* Trimiterea cererii GET pt descarcarea paginii */
  				memset (buffer, 0, BUFLEN);
  				sprintf (buffer, "GET %s HTTP/1.0\nHost: %s\n\n", url, newBuffer);

  					
  					/* Conexiunea http */

  				/* Obtinere descriptor de fisier - socket folosit 
				pentru conectarea la serverul http */

				sockhttp = socket (AF_INET, SOCK_STREAM, 0);
				if (sockhttp < 0){
					printErr(toFile, "Eroare: nu se deschide socketul\n");
					return -1;
				}

				/* Setare informarii pentru socket */
				memset(&serv_addr, 0, sizeof(serv_addr));
  				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(80);
				
				/* Determinare adresa ip a serverului http */
				struct hostent *host = gethostbyname(newBuffer);
				serv_addr.sin_addr = (struct in_addr&) *host->h_addr_list[0];
	
				/* Stabilirea conexiunii */	
				if (connect(sockhttp, (struct sockaddr*)
					&serv_addr, sizeof(serv_addr)) < 0){
				
					printErr(toFile, "Eroare: la conectarea cu serverul\n");		
					return -1;
				}

				/* Adaugam noul socket in multimea de descriptori */
				FD_SET (sockhttp, &read_fds);

				/* Trimitere cerere GET la serverul http */
				if (send (sockhttp, buffer, strlen(buffer), 0) < 0){
					printErr(toFile, "Eroare: scriere date pe socket\n");
				}

				/* Anuntam serverul ca urmeaza descarcarea paginii cerute */
				memset (buffer, 0, BUFLEN);
				sprintf (buffer, "start-download %s%s", newBuffer, url);

				if (send(sockfd , buffer, strlen(buffer), 0) < 0){
					printErr(toFile, "Eroare: scriere date pe socket\n");
				}

				strcpy (buffer, buffer + 15);
				memset (newBuffer, 0, BUFLEN);
				sprintf (newBuffer, "Pagina http://%s este in curs de descarcare..\n", buffer);

				printOut (toFile, newBuffer);
			}
		}

		/* Am primit date de pe socketul serverului http */
		if (FD_ISSET (sockhttp, &tmp_fds)){
			memset(buffer, 0 , BUFLEN);

			/* Citesc date de pe socket */
			int r = recv(sockhttp, buffer, sizeof(buffer), 0);

			if (r < 0){
				printErr (toFile, "Eroare: citire date de pe socket\n");
			}
			else if (r == 0){
				/* Trimiterea datelor a luat sfarsit */
				FD_CLR(sockhttp, &read_fds);
				close (sockhttp);
			}
					
			/* Eliminam din buffer raspunsul cererii GET
			si afisam doar sursa paginii */

			if (strncmp (buffer, "HTTP/1.1 200 OK", 15) == 0){
				pointer = strchr (buffer, '<');
				strcpy (buffer, buffer + (pointer - buffer));
			}

			/* Anuntam serverul ca s-a terminat descarcarea */
			if (r < 1){
				memset(buffer, 0, BUFLEN);
				strcpy(buffer, "stop-download");

				printOut (toFile, "Descarcarea s-a finalizat cu succes.\n");
			}

			if (send(sockfd , buffer, strlen(buffer), 0) < 0){
				printErr(toFile, "Eroare: Scriere date pe socket\n");
			}
		}
	}

	/* Inchidem fisierele daca au fost folosite */
	if (toFile == 1){
		fout.close();
		ferr.close();
	}

	return 0;
}
