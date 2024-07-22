Server:
	- deschide un socket UDP pentru a putea primi mesaje UDP si un socket TCP pentru a putea primi cereri de conexiune TCP de la clienti
	- porneste un poll pentru a asculta pe acesti socketi, pe stdin si pe socket-urile de la clienti pentru mesaje
	- daca primeste un mesaj pe socket-ul TCP, obtine si id-ul client-ului, verifica daca id-ul respectiv este deja conectat sau nu. In cazul in care nu este conectat, adauga socket-ul respectiv in poll, altfel inchide client-ul respectiv.
	- daca primeste un mesaj pe socket-ul UDP, verifica daca exista cineva abonat la topic-ul mesajului, si acel client este si conectat. Daca da, trimite mesajul mai departe.
	- daca primeste o comanda pe STDIN, verifica daca comanda este "exit". Daca da, inchide atat serverul, cat si toti clientii conectati.
	- daca primeste un mesaj pe un socket ce ii apartine unui client, verifica daca comanda este:
		- "exit": caz in care deconecteaza client-ul.
		- "subscribe topic": caz in care aboneaza client-ul la topic-ul respectiv, daca nu este deja abonat.
		- "unsubscribe topic": caz in care dezaboneaza client-ul de la topic-ul respectiv, daca este abonat.

Client:
	- la pornire, deschide un socket TCP pentru a comunica cu server-ul, si isi trimie id-ul.
	- porneste poll pentru a asculta mesaje atat de la server cat si de la STDIN.
	- daca primeste "exit" la stdin, inchide client-ul.
	- daca primeste mesaj de la server:
		- in cazul in care mesajul indica inchiderea client-ului, se inchide.
		- altfel afiseaza mesajul.

Incadrarea mesajelor:
	- la trimite, mesajele sunt incadrate, avand atasate la inceput lungimea lor.
	- la primire, mai intai se citeste lungimea mesajelor, si apoi se citeste mesajul de lungimea respectiva.
