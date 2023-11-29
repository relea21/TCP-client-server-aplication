Relea Florin Andrei 324CA

Aplicatie client-server TCP

Pentru a rezolva tema m-am folosit de rezolvarea laboratorului 7

Pentru a comunica peste TCP am creat un protocol care retine tipul de comanda
pentru a interactiona intre client si server 
Informatile despre protocol sunt stocate in structura messages
Tipurile sunt urmatoarele:
0 -> mesajul initial trimis de server pentru a-l anunta ce id are clientul
1 -> clientul trimite serverul mesaj de subscribe/unsubscribe
2 -> serverul cere unui client sa se inchida
3 -> serverul trimite unui client TCP ce a primit de la un client UDP unde era 
	abonat

In structura messages mai avem info_client care contine informatii despre 
clientul ce vrea sa se conecteze , client_action pentru a realiza operatia de
subscribe/unsubcribe, packet contine informatia pe care trebuie sa o proceseze 
un client TCP venita de la un client UDP.

In structura client_tcp_info : subscribed retine o lista cu toate topicurile la 
care un client este abonat, clientFD reprezinta file descriptorul pe care se afla 
clientul in server.



clients_logged -> un vector cu toti clientii care au fost conectati vreodata

