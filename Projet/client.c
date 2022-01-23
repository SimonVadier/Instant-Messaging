#include "pse.h"

#define CMD   "client"

void sessionMessages(int sock);
void afficherMenu(void);
void choixMenu(int sock);
void ajoutAmi(int sock);
void quitter(int sock);
void connexion(int sock);
void choixMessagerie(int sock);
void afficherEnLigne(int socket);
void afficherAmisEnLigne(int sock);
void messsagerie(int sock);

FILE *fdJournal;
char msgJournal[LIGNE_MAX];

int main(int argc, char *argv[]) {
  int sock, ret;
  struct sockaddr_in *adrServ;
	fdJournal = fopen("client.log", "a");
  if (fdJournal == NULL)
    erreur_IO("ouverture journal");

  if (argc != 3)
    erreur("usage: %s machine port\n", argv[0]); //Vérification des arguments 

  fprintf(fdJournal, "%s: creating a socket\n", CMD);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  fprintf(fdJournal, "%s: DNS resolving for %s, port %s\n", CMD, argv[1], argv[2]);
  adrServ = resolv(argv[1], argv[2]);
  if (adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);

  fprintf(fdJournal, "%s: adr %s, port %hu\n", CMD,
	        stringIP(ntohl(adrServ->sin_addr.s_addr)),
	        ntohs(adrServ->sin_port));
	
  fprintf(fdJournal, "%s: connecting the socket\n", CMD);
  ret = connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in));
  if (ret < 0)
    erreur_IO("connect");
	
	connexion(sock);
	while(VRAI) {
		afficherMenu();
		choixMenu(sock);
	}
  quitter(sock);
}

void connexion(int sock) {
	char nom[LG_PSEUDO];
	int lgEcr, lgLue;
	char ligne[LIGNE_MAX];
	
	fprintf(fdJournal, " - CONNEXION - \n");
	fprintf(fdJournal, "Veuillez saisir un nom d'utilisateur\n");
	fprintf(fdJournal, "nom : ");
	
	printf(" - CONNEXION - \n");
	printf("Veuillez saisir un nom d'utilisateur\n");
	printf("nom : ");
	
	if (fgets(nom, LG_PSEUDO, stdin) == NULL)
		erreur("Nom incorrect\n");
		
	lgEcr = ecrireLigne(sock, nom);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
	
	//printf("%s: %d bytes sent\n", CMD, lgEcr);
	lgLue = lireLigne(sock, ligne);
	if (lgLue < 0)
	  erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption server\n");
	
	//printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);	
	
	if (strncmp(ligne, nom, lgEcr-1) == 0) {
		printf("\nBienvenue %s\n", nom);
		fprintf(fdJournal, "\nBienvenue %s\n", nom);
	}
	else {
		printf("Erreur, veuillez reessayer\n");
		fprintf(fdJournal, "Erreur, veuillez reessayer\n");
		exit(EXIT_SUCCESS);  
	}
}

void sessionMessages(int sock) {
	int fin = FAUX;
  char ligne[LIGNE_MAX] ;
  int lgEcr, lgLue;
  
  while (!fin) {
    printf("ligne> ");
    fprintf(fdJournal, "ligne> ");
    if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
      erreur("saisie fin de fichier\n");

    lgEcr = sendLigne(sock, ligne);
    if(lgEcr == -1)
			erreur_IO("ecrire ligne");

    printf("%s: %d bytes sent\n", CMD, lgEcr);
    fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);

    if (strcmp(ligne, "fin\n") == 0)
      fin = VRAI;
      
      
    lgLue = recvLigne(sock, ligne);
    if (lgLue < 0)
      erreur_IO("lireLigne");
    else if (lgLue == 0)
      erreur("interruption server\n");

    printf("lgLue = %d\n", lgLue);
    fprintf(fdJournal, "lgLue = %d\n", lgLue);
		
  }

  if (close(sock) == -1)
    erreur_IO("close socket");
    
  exit(EXIT_SUCCESS);  
}

void afficherMenu(void) {
	
	printf("\n====== MENU PRINCIPAL ======\n");
	printf("1- Ajouter un ami\n");
	printf("2- Commencer une discution\n");
	printf("3- Quitter\n");
	fprintf(fdJournal, "\n====== MENU PRINCIPAL ======\n");
	fprintf(fdJournal, "1- Ajouter un ami\n");
	fprintf(fdJournal, "2- Commencer une discution\n");
	fprintf(fdJournal, "3- Quitter\n");
	return;
}

void choixMenu(int sock) {
	char choix_ch[LIGNE_MAX];
	int choix;
	printf("Choix : ");
	scanf("%s", choix_ch);
	
	choix = (int)choix_ch[0] - (int)'0';
	switch(choix){  	
  case 1: 
  	ajoutAmi(sock);
  	break;
	case 2:
		choixMessagerie(sock);
		break;
	case 3:
		quitter(sock);

		break;
	default:
			printf("\nBad request\n");
			fprintf(fdJournal, "\nBad request\n");
			afficherMenu();
			choixMenu(sock);
	}
	
}

void ajoutAmi(int sock) {
	char ligne[LIGNE_MAX] = "add";
  int lgEcr, lgLue;

	lgEcr = ecrireLigne(sock, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");  
	fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);
	
	afficherEnLigne(sock);
	
	fprintf(fdJournal, "Entrez l'id de la personne que vous souhaitez ajouter:\n#");
	printf("Entrez l'id de la personne que vous souhaitez ajouter:\n#");
	scanf("%s", ligne);
  
  lgEcr = ecrireLigne(sock, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");  
	fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);
	
	lgLue = lireLigne(sock, ligne);
	if (lgLue < 0)
	  erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption server\n");
	  
	fprintf(fdJournal, "%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	if (strcmp(ligne, "ok") == 0) {
		printf("\nAmi ajoute, retour au menu\n");
		fprintf(fdJournal, "\nAmi ajoute, retour au menu\n");
  }
  else if (strcmp(ligne, "erreur") == 0) {
  	fprintf(fdJournal, "Erreur, veuillez ressayer.\nRetour au menu\n");    
  	printf("Erreur, veuillez ressayer.\nRetour au menu\n");    
  }  
  else if (strcmp(ligne, "self") == 0) {
  	fprintf(fdJournal, "\nVous ne pouvez pas vous ajouter en amis.\nRetour au menu\n");    
  	printf("\nVous ne pouvez pas vous ajouter en amis.\nRetour au menu\n");    
  }  
}

void choixMessagerie(int sock) {
	char ligne[LIGNE_MAX] = "msg";
  int lgEcr, lgLue;

	lgEcr = ecrireLigne(sock, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");  
	fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);
	
	afficherAmisEnLigne(sock);
	
	fprintf(fdJournal, "Entrez l'id de la personne avec qui vous voulez discuter:\n#");
	printf("Entrez l'id de la personne avec qui vous voulez discuter:\n#");
	scanf("%s", ligne);
  
  lgEcr = ecrireLigne(sock, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");  
	fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);
	
	lgLue = lireLigne(sock, ligne);
	if (lgLue < 0)
	  erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption server\n");
	  
	fprintf(fdJournal, "%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	if (strcmp(ligne, "ok") == 0) {
		messsagerie(sock);
  }
  else if (strcmp(ligne, "erreur") == 0) {
  	fprintf(fdJournal, "Erreur, veuillez ressayer.\nRetour au menu\n");    
  	printf("Erreur, veuillez ressayer.\nRetour au menu\n");    
  }  
  else if (strcmp(ligne, "self") == 0) {
  	fprintf(fdJournal, "\nVous ne pouvez pas discuter avec vous meme.\nRetour au menu\n");    
  	printf("\nVous ne pouvez pas  discuter avec vous meme.\nRetour au menu\n");    
  }  
}

/*
	Lis les donnes envoyees par la fonction afficherClients du serveur 
*/
void afficherEnLigne(int sock) {
	char ligne[LIGNE_MAX];
  int lgLue;
  int nbClients;
  
	lgLue = lireLigne(sock, ligne);	//Demande au serveur le nombre de personnes a afficher
	if (lgLue < 0)
	  erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption server\n");
	
	fprintf(fdJournal, "%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	if (strcmp(&ligne[0], "0") == 0) {	//Vérifie si le nombre de workers est superieur à 10
		nbClients = ligne[1] - (int)'0'; //Convertit la chaine de caractere en un entier
	}
	else {
		nbClients = (ligne[0] - (int)'0')*10 + (ligne[1] - (int)'0'); //Convertit la chaine de caractere en un entier
	}

	printf("\nIl y a %d personnes en ligne :\n", nbClients);
	fprintf(fdJournal, "\nIl y a %d personnes en ligne :\n", nbClients);
	for (int i = 0; i < nbClients; i++) {
		lgLue = lireLigne(sock, ligne);
		if (lgLue < 0)
	  	erreur_IO("lireLigne");
		else if (lgLue == 0)
	 		erreur("interruption server\n");
		printf(" - %s\n", ligne);
		fprintf(fdJournal, " - %s\n", ligne);
	}
}

/*
	Lis les donnes envoyees par la fonction afficherAmis du serveur 
*/
void afficherAmisEnLigne(int sock) {
	char ligne[LIGNE_MAX];
  int lgLue;
  int nbClients;
  
	lgLue = lireLigne(sock, ligne);	//Demande au serveur le nombre de personnes a afficher
	if (lgLue < 0)
	  erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption server\n");
	
	fprintf(fdJournal, "%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	if (strcmp(&ligne[0], "0") == 0) {	//Vérifie si le nombre de workers est superieur à 10
		nbClients = ligne[1] - (int)'0'; //Convertit la chaine de caractere en un entier
	}
	else {
		nbClients = (ligne[0] - (int)'0')*10 + (ligne[1] - (int)'0'); //Convertit la chaine de caractere en un entier
	}

	printf("\nIl y a %d amis en ligne :\n", nbClients);
	fprintf(fdJournal, "\nIl y a %d amis en ligne :\n", nbClients);
	for (int i = 0; i < nbClients; i++) {
		lgLue = lireLigne(sock, ligne);
		if (lgLue < 0)
	  	erreur_IO("lireLigne");
		else if (lgLue == 0)
	 		erreur("interruption server\n");
		printf(" - %s\n", ligne);
		fprintf(fdJournal, " - %s\n", ligne);
	}
}

/*
	Ferme le .log et demande la deconexion
*/
void quitter(int sock) {
	char ligne[LIGNE_MAX] = "fin";
	int lgEcr;
	
	lgEcr = ecrireLigne(sock, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
	fprintf(fdJournal, "%s: %d bytes sent\n", CMD, lgEcr);
	
	fprintf(fdJournal, "\n - DECONNEXION - \n");
	printf("\n - DECONNEXION - \n"); //Annonce de fin de session
	
	if (fclose(fdJournal) == -1)
    erreur_IO("fermeture journal");
	exit(EXIT_SUCCESS);
}

void messsagerie(int sock) {
	//code de la documentation de select()
	fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);

	char recu[LIGNE_MAX] = "\0", envoye[LIGNE_MAX];
  int lgLue, lgEcr;
  int fin = FAUX;
  int retval;
  
  printf("En attente de votre correspondant\n");
  fprintf(fdJournal, "En attente de votre correspondant\n");
  
  lgLue = lireLigne(sock, recu);
  
  printf("Salon de discussion ouvert\n\n");
  while (!fin) {
		retval = select(1, &fds, NULL, NULL, NULL);  	
		if (retval) {
			if (fgets(envoye, LIGNE_MAX, stdin) == NULL)
		    erreur("saisie fin de fichier\n");
			if (strlen(envoye) > 1) {
		  	lgEcr = ecrireLigne(sock, envoye);
				if (strncmp(envoye, "fin", 3) == 0) {
					fin = VRAI; 
				}
				strcpy(envoye, "\0");
		  }
  	}
  	else if (retval == -1) {
  		erreur_IO("select()");
  	}		
		lgLue = sendLigne(sock, recu);
		if (lgLue > 1) {
		  printf("\nRecu :%s\n", recu[LIGNE_MAX]);
		}
  }
}


