
	Marcu Denis-George
	322CA

                               Web Crawler


    1. Detalii despre protocoalele folosite

    Am folosit protocolul TCP deoarece formeaza o conexiune permanenta intre
client si server. De asemenea, mesajele sunt primite in ordine, astfel ca nu
se vor pierde pachete.
    Pentru implementarea conexiunii TCP am avut drept exemplu scheletul de cod
pentru laboratorul 8 - TCP. MultiplexareIO

    CLIENT: Dupa stabilirea conexiunii TCP intre client si server, am folosit
functia select() pentru a determina daca clientul primeste date de la
socketul prin care s-a conectat cu serverul sau de la socketul prin care
s-a conectat la serverul http, daca s-a primit comanda de descarcare.

    SERVER: Folosim functia select() pentru a determina de unde se primesc
date. La server se pot conecta mai multi clienti deodata, astfel ca pentru
fiecare client conectat, se verifica daca pe socketul asociat conexiunii
dintre client si server exista date.

    
    Pentru descarcarea paginii web am folosit protocolul http. Obtinem adresa
ip a paginii web cu ajutorul functiei gethostbyname() si ne vom conecta la
serverul http prin portul 80.

    Dupa stabilirea conexiunii, se trimite cererea get si se incepe
descarcarea paginii web. Clientul va selecta socketul serverului hhtp, iar
acesta contine sursa paginii cerute.


    2. Fisiere

    Daca optiunea "-o" este activa, toate afisarile vor fi in doua fisiere
corespunzatoare stdout-ului si stderr-ului.

    Pentru client, la numele fisierului am adaugat si pid-ul procesului curent.
    

    3. Comanda status

    In programul server, la adaugarea unei noi conexiuni client-server, se
adauga intr-un vector noul client. Am construit o structura pentru memorarea
acestui client, in care vom retine adresa ip, portul si socketul prin care
este conectat.

    La citirea comenzii status, serverul va cauta daca exista clienti salvati
in vectorul descris mai sus. In caz ca exista, il va afisa, iar daca nu, va 
afisa un mesaj corespunzator.

    
    4. Comanda exit

    Serverul citeste comanda si trimite tuturor clientilor conectati mesaj
ca acesta se va inchide.

    Inainte de a se inchide serverul, vom sterge din multimea de descriptori
toti clientii si vom inchide conexiunile cu acestia. Se goleste lista de
clienti activi si se termina executia programului server.

    Clientul va primi pe socketul serverului comanda de exit. In acest caz
se va intrerupe conexiunea cu serverul, stergem din multimea de descriptori
socketul asociat si oprim conexiunea si executia programului client.


    5. Comanda download

    Serverul citeste comanda de download.

    Daca exista clienti conectati se trimite comanda catre primul client
gasit (vom trimite doar adresa url).

    Iar daca nu exista niciun client conectat, se asteapta conectarea unui
client. Cand se intampla acest lucru, odata cu mesajul de confirmarea a
conexiunii vom trimite si comanda de download (doar adresa url).  

    Clientul va primi una din comenzile descrise anterior. Dupa realizarea
conexiunii, clientul verifica daca dupa mesajul standard de confirmare,
"Clientul s-a conectat cu succes.", mai exista ceva in buffer (adresa url).

    Daca bufferul contine la inceput indicatorul "http://" inseamna ca s-a
dat comanda de download. Extragem din adresa url numele hostului si calea
catre pagina dorita si apelam cererea GET catre serverul http. De asemenea,
anuntam serverul ca descarcarea a inceput.

    Serverul primeste mesajul si va crea o ierarhie de directoare conform
adresei url si va salva datele primite de la client in fisierul corespunzator.

    In client functia select() va arata catre socketul serverului http. Astfel
primim date cu codul sursa al paginii dorite. Aceste date le vom trimite
serverului, care le va scrie in fisierul corespunzator adresei url.

    Dupa ce s-a terminat descarcarea paginii (nu mai avem date pe socketul
legat la serverul http), asa ca vom anunta serverul de acest lucru si vom
inchide conexiunea clientului cu serverul http.

    Serverul receptioneaza mesajul si va inchide fisierul in care s-a
descarcat pagina web.

  Mai multe detalii despre implementare se gasesc in comentariile din cod.
