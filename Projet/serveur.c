#include "pse.h"

#define   CMD         "serveur"
#define   NB_WORKERS   50

void creerCohorteWorkers(void);
int chercherWorkerLibre(void);
void *threadWorker(void *arg);
void sessionClient(int canal, int tid);
void connexionClient(DataSpec *dataTh);
int listeClients(int *liste);
void ajoutAmi(int canal, int tid);
int ecrireJournal(char *buffer);
void remiseAZeroJournal(void);
void lockMutexJournal(void);
void unlockMutexJournal(void);
void lockMutexCanal(int numWorker);
void unlockMutexCanal(int numWorker);
void inttoChar(char* chaine, int nombre);
int chartoInt(char* chaine);
void afficherClients(int canal);
void choixMessagerieClient(int canal, int tid);
void initTabs(void);
void messagerieClient(int id1, int id2);

int fdJournal;
DataSpec dataSpec[NB_WORKERS];
sem_t semWorkersLibres;
int tabAmis[NB_WORKERS][NB_WORKERS];
int tabMessagerie[NB_WORKERS][NB_WORKERS];

// la remise a zero du journal modifie le descripteur du fichier, il faut donc
// proteger par un mutex l'ecriture dans le journal et la remise a zero
pthread_mutex_t mutexJournal;

// pour l'acces au canal d'un worker peuvant etre en concurrence la recherche
// d'un worker libre et la mise à -1 du canal par le worker
pthread_mutex_t mutexCanal[NB_WORKERS];

int main(int argc, char *argv[]) {
  short port;
  int ecoute, canal, ret;
  struct sockaddr_in adrEcoute, adrClient;
  unsigned int lgAdrClient;
  int numWorkerLibre;
  
  //INITIALISATION (Worker, Logs, Semaphores, Connexion, Amis)
  fdJournal = open("journal.log", O_WRONLY|O_CREAT|O_APPEND, 0644);
  if (fdJournal == -1)
    erreur_IO("ouverture journal");

  creerCohorteWorkers(); //init worker
  
  ret = sem_init(&semWorkersLibres, 0, NB_WORKERS);//init semaphores
  if (ret == -1)
    erreur_IO("init sem workers libres");

  if (argc != 2) //Verification des arguments
    erreur("usage: %s port\n", argv[0]);
	
	initTabs(); //Initialisation du tableau Amis
	
  port = (short)atoi(argv[1]);

  printf("%s: creating a socket\n", CMD);
  ecoute = socket (AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(port);
  printf("%s: binding to INADDR_ANY address on port %d\n", CMD, port);
  ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  printf("%s: listening to socket\n", CMD);
  ret = listen (ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");
  
  while (VRAI) {
    printf("%s: accepting a connection\n", CMD);
    canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
    if (canal < 0)
      erreur_IO("accept");

    printf("%s: adr %s, port %hu\n", CMD,
	      stringIP(ntohl(adrClient.sin_addr.s_addr)), ntohs(adrClient.sin_port));

    ret = sem_wait(&semWorkersLibres);  // attente d'un worker libre
    if (ret == -1)
      erreur_IO("wait sem workers libres");
    numWorkerLibre = chercherWorkerLibre();

    dataSpec[numWorkerLibre].canal = canal;
    sem_post(&dataSpec[numWorkerLibre].sem);  // reveil du worker
    if (ret == -1)
      erreur_IO("post sem worker");
  }

  if (close(ecoute) == -1)
    erreur_IO("fermeture ecoute");

  if (close(fdJournal) == -1)
    erreur_IO("fermeture journal");

  exit(EXIT_SUCCESS);
}

//Crée NB_WORKER threads pouvant accepter une connexion
void creerCohorteWorkers(void) {
  int i, ret;

  for (i= 0; i < NB_WORKERS; i++) {
    dataSpec[i].canal = -1;
    dataSpec[i].tid = i;
    ret = sem_init(&dataSpec[i].sem, 0, 0);
    if (ret == -1)
      erreur_IO("init sem worker");

    ret = pthread_create(&dataSpec[i].id, NULL, threadWorker, &dataSpec[i]);
    if (ret != 0)
      erreur_IO("creation thread");
  }
}
// retourne le no. du worker libre trouve ou -1 si pas de worker libre
int chercherWorkerLibre(void) {
  int numWorkerLibre = -1, i = 0, canal;

  while (numWorkerLibre < 0 && i < NB_WORKERS) {
    lockMutexCanal(i);
    canal = dataSpec[i].canal;
    unlockMutexCanal(i);

    if (canal == -1)
      numWorkerLibre = i;
    else
      i++;
  }

  return numWorkerLibre;
}

void *threadWorker(void *arg) {
  DataSpec *dataTh = (DataSpec *)arg;
  int ret;

  while (VRAI) {
    ret = sem_wait(&dataTh->sem); // attente reveil par le thread principal
    if (ret == -1)
      erreur_IO("wait sem worker");
    printf("%s: worker %d reveil\n", CMD, dataTh->tid);
    
		for (int i = 0; i < NB_WORKERS; i++) {
			tabAmis[dataTh->tid][i] = 0;
		}
	  connexionClient(dataTh);
    sessionClient(dataTh->canal, dataTh->tid);

    lockMutexCanal(dataTh->tid);
    dataTh->canal = -1;
    unlockMutexCanal(dataTh->tid);

    ret = sem_post(&semWorkersLibres);  // incrementer le nb de workers libres
    if (ret == -1)
      erreur_IO("post sem workers libres");

    printf("%s: worker %d sommeil\n", CMD, dataTh->tid);
  }

  pthread_exit(NULL);
}

/*
	Menu principal
*/ 
void sessionClient(int canal, int tid) {
  int fin = FAUX;
  char ligne[LIGNE_MAX];
  int lgLue, lgEcr;
  while (!fin) {
    lgLue = lireLigne(canal, ligne);
    if (lgLue < 0)
      erreur_IO("lireLigne");
    else if (lgLue == 0)
      erreur("interruption client\n");
      
    printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
      
    if (strcmp(ligne, "fin") == 0) { //demande de fermeture du serveur
    	printf("%s: fin client\n", CMD);
    	fin = VRAI;
    }
    else if (strcmp(ligne, "add") == 0) {
    	ajoutAmi(canal, tid);
    }
    else if (strcmp(ligne, "msg") == 0) {
    	choixMessagerieClient(canal, tid);
    }
    else
      erreur_IO("choix menu");
  } // fin while

  if (close(canal) == -1)
    erreur_IO("fermeture canal");
}

/* 
	Demande le pseudo de l'utilisateur et l'associe au worker
*/ 
void connexionClient(DataSpec *dataTh) {
  char ligne[LIGNE_MAX];
  int lgLue, lgEcr;
	lgLue = lireLigne(dataTh->canal, ligne);
	if (lgLue < 0)
		erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption client\n");
		    
	printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
		    
	strcpy(dataTh->pseudo, ligne);
	
	lgEcr = ecrireLigne(dataTh->canal, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
	
	printf("%s: %d bytes sent\n", CMD, lgEcr);
}

/*
Retourne le nombre de personnes connectes et ecrit dans liste le numero de worker occupe 
Remarque: Peu optimise, on pourrait changer dynamiquement un tableau de pseudo 
					en meme temps que la modification des canaux mais pour 50 workers
					ce n'est pas vraiment necessaire
*/
int listeClients(int *liste) {
	int nbClients = 0;
	for (int i = 0; i < NB_WORKERS; i++) {
		if (dataSpec[i].canal != -1) {
			liste[nbClients] = i;
			nbClients++;
		}
	}
	return nbClients;
}

/*
	Envoie la liste des personnes connectes puis demande la personne a ajouter en amis
*/
void ajoutAmi(int canal, int tid) {
	char ligne[LIGNE_MAX];
  int lgLue, lgEcr;
  int id;
  afficherClients(canal);
  
  lgLue = lireLigne(canal, ligne);
	if (lgLue < 0)
		erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption client\n");
		    
	printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	id = chartoInt(ligne);
	if (id == tid) {
		strcpy(ligne, "self");
	}
	else if (dataSpec[id].canal != -1) {
		tabAmis[tid][id] = 1;
		strcpy(ligne, "ok");
	}
	else {
		strcpy(ligne, "erreur");
	}

	lgEcr = ecrireLigne(canal, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
}

/* 
	Affiche la liste des personnes connectes et leur identifiant
*/
void afficherClients(int canal) {
	int nbClients;
	int clientsActifs[NB_WORKERS];
	char ligne[LIGNE_MAX];
  int lgEcr;
  char chr_nbClients[LIGNE_MAX] = "00";
  char id[LIGNE_MAX] = "00";
  
	nbClients = listeClients(clientsActifs);
	printf("nbClients = %d\n", nbClients);
	inttoChar(chr_nbClients, nbClients);
	strcpy(ligne, chr_nbClients);
	
	lgEcr = ecrireLigne(canal, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
	
	printf("%s: %d bytes sent: %s\n", CMD, lgEcr, ligne);

	for (int i = 0; i < nbClients; i++) {
		inttoChar(id, dataSpec[clientsActifs[i]].tid);
		strcpy(ligne, dataSpec[clientsActifs[i]].pseudo);	
		strcat(ligne, "#");
		strcat(ligne, id);
		lgEcr = ecrireLigne(canal, ligne);
		if(lgEcr == -1)
			erreur_IO("ecrire ligne");
	}
}

/* 
	Affiche la liste des amis connectes et leur identifiant
*/
void afficherAmis(int canal, int tid) {
	int nbClients, nbAmis = 0;
	int clientsActifs[NB_WORKERS], amisActifs[NB_WORKERS];
	char ligne[LIGNE_MAX];
  int lgEcr;
  char chr_nbClients[LIGNE_MAX] = "00";
  char id[LIGNE_MAX] = "00";
  
	nbClients = listeClients(clientsActifs);
	for (int i = 0; i < nbClients; i++) {
		if (tabAmis[tid][clientsActifs[i]]) { //Ne garde que les personnes actives qui sont dans les amis
			amisActifs[nbAmis] = clientsActifs[i];
			nbAmis++;
		}
	}
	printf("nbAmis = %d\n", nbAmis);
	inttoChar(chr_nbClients, nbAmis);
	strcpy(ligne, chr_nbClients);
	
	lgEcr = ecrireLigne(canal, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
	
	printf("%s: %d bytes sent\n", CMD, lgEcr);

	for (int i = 0; i < nbAmis; i++) {
		inttoChar(id, dataSpec[amisActifs[i]].tid);
		strcpy(ligne, dataSpec[amisActifs[i]].pseudo);	
		strcat(ligne, "#");
		strcat(ligne, id);
		lgEcr = ecrireLigne(canal, ligne);
		if(lgEcr == -1)
			erreur_IO("ecrire ligne");
	}
}

void choixMessagerieClient(int canal, int tid) {
	char ligne[LIGNE_MAX];
  int lgLue, lgEcr;
  int id;
  int possible = FAUX;
  afficherAmis(canal, tid);
  
  lgLue = lireLigne(canal, ligne);
	if (lgLue < 0)
		erreur_IO("lireLigne");
	else if (lgLue == 0)
	  erreur("interruption client\n");
		    
	printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
	
	id = chartoInt(ligne);
	if (id == tid) {
		strcpy(ligne, "self");
	}
	else if (dataSpec[id].canal > -1) {
		tabAmis[tid][id] = 1;
		strcpy(ligne, "ok");
		possible = VRAI;
	}
	else {
		strcpy(ligne, "erreur");
	}

	lgEcr = ecrireLigne(canal, ligne);
	if(lgEcr == -1)
		erreur_IO("ecrire ligne");
		
	if(possible) {
		tabMessagerie[tid][id] = 1;
		messagerieClient(tid, id);		
	}
}

/*
	Messagerie entre deux workers
*/
void messagerieClient(int id1, int id2) {
	char ligne[LIGNE_MAX];
  int lgLue, lgEcr;
  int debut = FAUX, fin = FAUX;
  while (!debut) {
  	if (tabMessagerie[id1][id2] && tabMessagerie[id2][id1])
  		debut = VRAI;
  }
  strcpy(ligne, "ok");
  lgEcr = ecrireLigne(dataSpec[id1].canal, ligne);
  while (!fin) {
  	lgLue = lireLigne(dataSpec[id1].canal, ligne);
  	printf("%s: reception %d octets : \"%s\"\n", CMD, lgLue, ligne);
  	if (lgLue < 0) {
	  	erreur_IO("lireLigne");
		}
		else if (lgLue == 0) {
	 		erreur("interruption client\n");
		}
		else {
		  lgEcr = sendLigne(dataSpec[id2].canal, ligne);
		  printf("%s: %d bytes sent: \"%s\"\n", CMD, lgEcr, ligne);
			if (strncmp(ligne, "fin", 3) == 0) {
				fin = VRAI; 
			}
		}
  }
  tabMessagerie[id1][id2] = 0;
}

/*
	Ecrit nombre dans chaine sous le format : 
	0x si nombre < 10
	x si nombre >= 10
*/
void inttoChar(char* chaine, int nombre) {
	if (nombre < 9) {
		chaine[1] =  nombre + '0';
		chaine[0] =  '0';
	}
	else {
		chaine[0] =  (nombre/10)%10 + '0';
		chaine[1] =  nombre%10 + '0';
	}
} 

/*
	Convertit les deux premiers caracteres de chaine en un entierpthread_mutex_t mutexJournal;
*/
int chartoInt(char* chaine) {
	int nombre;
	if (strcmp(&chaine[0], "0") == 0) {	//Vérifie si le nombre est superieur à 10
		nombre = chaine[1] - (int)'0'; //Convertit la chaine de caractere en un entier
	}
	else {
		nombre = (chaine[0] - (int)'0')*10 + (chaine[1] - (int)'0'); //Convertit la chaine de caractere en un entier
	}
	return nombre;
} 

int ecrireJournal(char *buffer) {
  int lgLue;

  lockMutexJournal();
  lgLue = ecrireLigne(fdJournal, buffer);
  unlockMutexJournal();
  return lgLue;
}

// le fichier est ferme et rouvert vide
void remiseAZeroJournal(void) {
  lockMutexJournal();

  if (close(fdJournal) == -1)
    erreur_IO("raz journal - fermeture");

  fdJournal = open("journal.log", O_WRONLY|O_TRUNC|O_APPEND, 0644);
  if (fdJournal == -1)
    erreur_IO("raz journal - ouverture");

  unlockMutexJournal();
}

void lockMutexJournal(void)
{
  int ret;

  ret = pthread_mutex_lock(&mutexJournal);
  if (ret != 0)
    erreur_IO("lock mutex journal");
}

void unlockMutexJournal(void)
{
  int ret;

  ret = pthread_mutex_unlock(&mutexJournal);
  if (ret != 0)
    erreur_IO("lock mutex journal");
}

void lockMutexCanal(int numWorker)
{
  int ret;

  ret = pthread_mutex_lock(&mutexCanal[numWorker]);
  if (ret != 0)
    erreur_IO("lock mutex dataspec");
}

void unlockMutexCanal(int numWorker)
{
  int ret;

  ret = pthread_mutex_unlock(&mutexCanal[numWorker]);
  if (ret != 0)
    erreur_IO("lock mutex dataspec");
}

/*
	Initialise le tableau tabAmis a 0
*/
void initTabs(void){
	for (int i = 0; i < NB_WORKERS; i++) {
		for (int j = 0; j < NB_WORKERS; j++) {
			tabAmis[i][j] = 0;
			tabMessagerie[i][j] = 0;
		}
	}
}
