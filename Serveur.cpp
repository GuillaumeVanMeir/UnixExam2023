#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include <cerrno>

#include "FichierUtilisateur.h"

int idQ, idShm, idSem;
TAB_CONNEXIONS *tab;

void afficheTab();

MYSQL *connexion;
void Handler(int sig);
void SendAddUser(MESSAGE sender);
void SendRemoveUser(MESSAGE sender);

int main()
{
  // Connection à la BD
  int TempPid = 0;
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL)
  {
    fprintf(stderr, "(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);
  }

  // Armement des signaux
  struct sigaction A;
  A.sa_handler = Handler;
  sigemptyset(&A.sa_mask);
  A.sa_flags = 0;
  if (sigaction(SIGINT, &A, NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }
  if (sigaction(SIGCHLD, &A, NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  // Creation des ressources
  fprintf(stderr, "(SERVEUR %d) Creation de la file de messages\n", getpid());
  if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0600)) == -1) // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Initialisation du tableau de connexions
  fprintf(stderr, "(SERVEUR %d) Initialisation de la table des connexions\n", getpid());
  tab = (TAB_CONNEXIONS *)malloc(sizeof(TAB_CONNEXIONS));


  for (int i = 0; i < 6; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom, "");
    for (int j = 0; j < 5; j++)
      tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }
  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();

  // Initialisation de la sémaphore
  if ((idSem = semget(CLE,1,IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    perror("(SERVEUR) Erreur de semget");
    exit(1);
  }
  if (semctl(idSem,0,SETVAL,1) == -1)
  {
    perror("Erreur de semctl (1)");
    exit(1);
  }
  // Initialisation de la mémoir partagé
  if ((idShm = shmget(CLE, 200 * sizeof(char), IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    perror("Erreur de shmget");
    exit(1);
  }

  // Creation du processus Publicite
  int i, k, j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES *resultat;
  MYSQL_ROW tuple;
  PUBLICITE pub;
  int idPub;
  int fd;


  if ((idPub = fork()) == -1)
  {
    perror("Erreur de fork()");
    exit(1);
  }

  if (idPub == 0)
  {
    //processus fils Publicite
    if (int r = execl("./Publicite", NULL) == -1)
    {
      perror("Erreur de execl()");
      exit(1);
    }
  }
  else{
    tab->pidPublicite = idPub;
  }

  while (1)
  {
    fprintf(stderr, "(SERVEUR %d) Attente d'une requete...\n", getpid());
    //reception d'un message
    RCV_SERV:
    if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), 1, 0) == -1)
    {
      //en cas d'arret de la fonction à cause d'un signal utilisation de jump
      if (errno == EINTR)
        goto RCV_SERV;
      msgctl(idQ, IPC_RMID, NULL);
      shmctl(idShm, IPC_RMID, NULL);
      perror("(SERVEUR) Erreur de msgrcv");
      exit(1);
    }

    switch (m.requete)
    {
    case CONNECT:
      // Ajout du pid d'un client dans la tab de connexion
      fprintf(stderr, "(SERVEUR %d) Requete CONNECT reçue de %d\n", getpid(), m.expediteur);
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == 0)
        {
          tab->connexions[i].pidFenetre = m.expediteur;
          i = 6;
        }
      }

      break;

    case DECONNECT:
      //Suppression du pid du client dans la table de connexion 
      fprintf(stderr, "(SERVEUR %d) Requete DECONNECT reçue de %d\n", getpid(), m.expediteur);
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == m.expediteur)
        {
          tab->connexions[i].pidFenetre = 0;
          i = 6;
        }
      }
      break;

    case LOGIN:
      char phraseRetour[50], okORko[3];

      fprintf(stderr, "(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n", getpid(), m.expediteur, m.data1, m.data2, m.texte);

      if (strcmp(m.data1, "1") == 0) // Nouveau Client
      {
        if (estPresent(m.data2) == 0) // Nouveau Client coché + non existant OU FICHIER NON EXISTANT
        {
          ajouteUtilisateur(m.data2, m.texte);
          strcpy(phraseRetour, "Nouveau client créé : bienvenue !\n");
          strcpy(okORko, "OK");

          //ajout du client dans la BD
          char requete[256];
          sprintf(requete, "insert into UNIX_FINAL values (NULL,'%s','%s','%s');", m.data2, "---", "---");
          MYSQL *connexion = mysql_init(NULL);
          mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0);
          mysql_query(connexion, requete);
          mysql_close(connexion);
        }
        else // Nouveau Client coché + existant
        {
          strcpy(phraseRetour, "Client déjà existant !\n");
          strcpy(okORko, "KO");
        }
      }
      else // NOUVEAU CLIENT NON COCHE
      {

        if (estPresent(m.data2) == 0) // Nouveau Client non coché + non existant
        {
          strcpy(phraseRetour, "Client inconnu...\n");
          strcpy(okORko, "KO");
        }
        else // Nouveau Client non coché + existant
        {
          if (verifieMotDePasse(estPresent(m.data2), m.texte) == -1)
          {
            strcpy(phraseRetour, "Le fichier n'existe pas\n");
            strcpy(okORko, "KO");
          }

          if (verifieMotDePasse(estPresent(m.data2), m.texte) == 1)
          {
            int clientconnect = 0; 
            //gestion de la demande de login avec un client déjà login
            for (int i = 0; i < 6; i++)
            {
              if (strcmp(m.data2,tab->connexions[i].nom) ==0){
                i=6;
                strcpy(phraseRetour, "Client deja connecte\n");
                strcpy(okORko, "KO");
                clientconnect = 1;
              }
            }
            if(clientconnect==0){ 
              strcpy(phraseRetour, "Re-bonjour cher client !\n");
              strcpy(okORko, "OK");
            }
          }

          if (verifieMotDePasse(estPresent(m.data2), m.texte) == 0)
          {
            strcpy(phraseRetour, "Mot de passe incorrect...\n");
            strcpy(okORko, "KO");
          }
        }
      }

      //initialisation du message de retour du LOGIN
      MESSAGE retourReponse;

      retourReponse.type = m.expediteur;
      retourReponse.expediteur = 1;
      retourReponse.requete = LOGIN;
      strcpy(retourReponse.data1, okORko);
      strcpy(retourReponse.texte, phraseRetour);

      if (msgsnd(idQ, &retourReponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("ICI1(Serveur) Erreur de msgsnd");
        exit(1);
      }

      
      if (kill(retourReponse.type, SIGUSR1) == -1)
      {
        perror("Erreur de kill");
        exit(1);
      }
      //ajout du nom de client dans la table de connexion
      if (strcmp(okORko, "OK") == 0)
      {
        SendAddUser(m);
        for (int i = 0; i < 6; i++)
        {
          if (tab->connexions[i].pidFenetre == m.expediteur)
          {
            strcpy(tab->connexions[i].nom, m.data2);
            i = 6;
          }
        }
      }

      break;

    case LOGOUT:
      // Suppression du nom de client dans la table de connexion
      fprintf(stderr, "(SERVEUR %d) Requete LOGOUT reçue de %d \n", getpid(), m.expediteur);
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == m.expediteur)
        {
          strcpy(m.data1, tab->connexions[i].nom);
          strcpy(tab->connexions[i].nom, "");
          for (int j = 0; j < 5; j++)
          {
            tab->connexions[i].autres[j] = 0;
          }
          i = 6;
        }
      }
      SendRemoveUser(m);
      for (int i = 0; i < 6; i++)
      {
        for (int j = 0; j < 5; j++)
        {
          if (m.expediteur == tab->connexions[i].autres[j])
            tab->connexions[i].autres[j] = 0;
        }
      }

      break;

    case ACCEPT_USER:
      TempPid = 0;
      fprintf(stderr, "(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n", getpid(), m.expediteur);
      //trouve le pid du client accepte en fonction de son nom
      for (int i = 0; i < 6; i++)
      {
        if (strcmp(m.data1, tab->connexions[i].nom) == 0)
        {
          TempPid = tab->connexions[i].pidFenetre;
          i = 6;
        }
      }
      //ajoute le pid du client accepte a l'autre client
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == m.expediteur)
        {
          for (int j = 0; j < 5; j++)
          {
            if (tab->connexions[i].autres[j] == 0)
            {
              tab->connexions[i].autres[j] = TempPid;
              j = 5;
            }
          }
          i = 6;
        }
      }

      break;

    case REFUSE_USER:
      fprintf(stderr, "(SERVEUR %d) Requete REFUSE_USER reçue de %d\n", getpid(), m.expediteur);

      // trouve le pid du client refuse en fonction de son nom
      for (int i = 0; i < 6; i++)
      {
        if (strcmp(m.data1, tab->connexions[i].nom) == 0)
        {
          TempPid = tab->connexions[i].pidFenetre;
          i = 6;
        }
      }

      // supprime le pid du client refuse a l'autre client
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == m.expediteur)
        {
          for (int j = 0; j < 5; j++)
          {
            if (tab->connexions[i].autres[j] == TempPid)
            {
              tab->connexions[i].autres[j] = 0;
              j = 5;
            }
          }
          i = 6;
        }
      }

      break;

    case SEND:
      fprintf(stderr, "(SERVEUR %d) Requete SEND reçue de %d\n", getpid(), m.expediteur);

      MESSAGE sendMessage2Clients;
      sendMessage2Clients.requete = SEND;
      sendMessage2Clients.expediteur = 1;
      strcpy(sendMessage2Clients.texte, m.texte);

      //trouve les clients qui ont accepté de discuté avec le client
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre == m.expediteur)
        {
          strcpy(sendMessage2Clients.data1, tab->connexions[i].nom);

          //envoie le message au différent client
          for (int j = 0; j < 5; j++)
          {
            if (tab->connexions[i].autres[j] != 0)
            {
              sendMessage2Clients.type = tab->connexions[i].autres[j];

              if (msgsnd(idQ, &sendMessage2Clients, sizeof(MESSAGE) - sizeof(long), 0) == -1)
              {
                perror("ICI2(Serveur) Erreur de msgsnd");
                exit(1);
              }
              if (kill(sendMessage2Clients.type, SIGUSR1) == -1)
              {
                perror("Erreur de kill");
                exit(1);
              }
            }
          }
        }
      }

      break;

    case UPDATE_PUB:
      fprintf(stderr, "(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n", getpid(), m.expediteur);
      tab->pidPublicite = m.expediteur;
      //envoie un signal à tous les clients
      for (int i = 0; i < 6; i++)
      {
        if (tab->connexions[i].pidFenetre != 0)
        {
          if (kill(tab->connexions[i].pidFenetre, SIGUSR2) == -1)
          {
            perror("Erreur de kill");
            exit(1);
          }
        }
      }
      break;

    case CONSULT:
      fprintf(stderr, "(SERVEUR %d) Requete CONSULT reçue de %d\n", getpid(), m.expediteur);

      int idConsult;
      if ((idConsult = fork()) == -1)
      {
        perror("Erreur de fork()");
        exit(1);
      }
      if (idConsult == 0)
      {
        //code du fils Consultation
        if (int r = execl("./Consultation", NULL) == -1)
        {
          perror("Erreur de execl()");
          exit(1);
        }
      }
      else if (idConsult != 0)
      {
        m.type = idConsult;
        //transfert du message au processus consultation
        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
        {
          perror("ICI(Serveur) Erreur de msgsnd");
          exit(1);
        }
      }

      break;

    case MODIF1:
      fprintf(stderr, "(SERVEUR %d) Requete MODIF1 reçue de %d\n", getpid(), m.expediteur);

      int idmodif;
      if ((idmodif = fork()) == -1)
      {
        perror("Erreur de fork()");
        exit(1);
      }
      if (idmodif == 0)
      {
        // code du fils Modification

        if (int r = execl("./Modification", NULL) == -1)
        {
          perror("Erreur de execl()");
          exit(1);
        }
      }
      else if (idmodif != 0)
      {
        m.type = idmodif;
        //recupère le nom du client qui fait la demande
        for (int i = 0; i < 6; i++)
        {
          if (m.expediteur == tab->connexions[i].pidFenetre)
          {
            strcpy(m.data1, tab->connexions[i].nom);
            tab->connexions[i].pidModification = idmodif;
            i = 6;
          }
        }
      }

      //envoie du message au processus modification
      if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("ICI3(Serveur) Erreur de msgsnd");
        exit(1);
      }

      break;

    case MODIF2:
      fprintf(stderr, "(SERVEUR %d) Requete MODIF2 reçue de %d\n", getpid(), m.expediteur);

      //recuperation du pid du processus modification
      for (int i = 0; i < 6; i++)
      {
        if (m.expediteur == tab->connexions[i].pidFenetre)
        {
          m.type = tab->connexions[i].pidModification;
          i = 6;
        }
      }

      // transfert du message au processus modification
      if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("ICI3(Serveur) Erreur de msgsnd");
        exit(1);
      }

      break;

    case LOGIN_ADMIN:
      fprintf(stderr, "(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n", getpid(), m.expediteur);
      //verification de la presence d'un admin
      if(tab->pidAdmin ==0){
        tab->pidAdmin = m.expediteur;
        strcpy(m.data1, "OK");
      }
      else
        strcpy(m.data1, "KO");
      m.type = m.expediteur;
      m.expediteur = getpid();

      if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("(SERVEUR) Erreur de msgsnd");
        exit(1);
      }
      break;

    case LOGOUT_ADMIN:
      fprintf(stderr, "(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n", getpid(), m.expediteur);
      tab->pidAdmin = 0;
      break;

    case NEW_USER:
      fprintf(stderr, "(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n", getpid(), m.expediteur, m.data1, m.data2);
    
      struct sembuf operation;
      operation.sem_num = 0;
      operation.sem_op = -1;
      operation.sem_flg = IPC_NOWAIT;
      //prise de la semaphore
      if (semop(idSem, &operation, 1) == -1)
      {
        strcpy(phraseRetour, "Erreur pas d'ajout ni de modif de mdp !\n");
        strcpy(okORko, "KO");
      }
      else
      { 
        fprintf(stderr, "(SERVEUR %d) Prise bloquante du sémaphore 0\n", getpid());

        int pos;
        if (estPresent(m.data1) == 0) //Client non existant
        {
          ajouteUtilisateur(m.data1, m.data2);

          //ajout de l'utilisateur à la BD
          char requete[256];
          sprintf(requete, "insert into UNIX_FINAL values (NULL,'%s','%s','%s');", m.data1, "---", "---");
          MYSQL *connexion = mysql_init(NULL);
          mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0);
          mysql_query(connexion, requete);
          mysql_close(connexion);
          
          strcpy(phraseRetour, "Nouveau client créé : bienvenue !\n");
          strcpy(okORko, "OK");
        }
        else if((pos=estPresent(m.data1)) != 0)//Client existant
        {
          //modifiction du mdp de l'utilisateur
          UTILISATEUR u;
          if(strcmp(m.data1,"")!=0)
          {
            int fd;

            if ((fd = open("utilisateurs.dat",O_RDWR,0664)) == -1)
            {
              perror("Erreur de fopen() dans Modification");
              exit(1);
            }      

            lseek(fd,(pos-1)*sizeof(UTILISATEUR), SEEK_SET);
            if ((read(fd,&u,sizeof(UTILISATEUR))) == -1) 
            {
              perror("Erreur de read(1) dans estPresent");
            }
            u.hash = hash(m.data2);
            lseek(fd,(pos-1)*sizeof(UTILISATEUR), SEEK_SET);
            write(fd,&u,sizeof(UTILISATEUR));

            if (close(fd))
            {
              perror("Erreur de fclose() dans Modification");
              exit(1);
            }
          }

          strcpy(phraseRetour, "Client déjà existant donc modif mdp !\n");
          strcpy(okORko, "OK");
        }
  
        operation.sem_op = 1;
        if (semop(idSem, &operation, 1) == -1)
          perror("(SERVEUR) Erreur de semop");
        fprintf(stderr, "(SERVEUR %d) Libération du sémaphore 0\n", getpid());
      }
      
      //envoie du message a admin
      m.type=m.expediteur;
      m.expediteur=getpid();
      m.requete=NEW_USER;
      strcpy(m.data1,okORko);
      strcpy(m.texte,phraseRetour);
      if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
      {
        perror("(SERVEUR) Erreur de msgsnd");
        exit(1);
      }

      break;

    case DELETE_USER:
      fprintf(stderr, "(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n", getpid(), m.expediteur, m.data1);

      operation.sem_num = 0;
      operation.sem_op = -1;
      operation.sem_flg = IPC_NOWAIT;
      if (semop(idSem, &operation, 1) == -1)
      {
        strcpy(phraseRetour, "Erreur pas de suppression !\n");
        strcpy(okORko, "KO");
      }
      else
      {
        fprintf(stderr, "(SERVEUR %d) Prise bloquante du sémaphore 0\n", getpid());

        int pos;
        if (estPresent(m.data1) == 0) //Client non existant
        {
          strcpy(phraseRetour, "Utilisateur introuvable !\n");
          strcpy(okORko, "KO");
        }
        else if((pos=estPresent(m.data1)) != 0) //Client existant
        {
          int fd;
          UTILISATEUR u;

          //supression de l'utilisateur dans la BD
          char requete[256];
          sprintf(requete, "DELETE FROM UNIX_FINAL WHERE UPPER(nom) LIKE UPPER('%s');",m.data1);
          MYSQL *connexion = mysql_init(NULL);
          mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0);
          mysql_query(connexion, requete);
          mysql_close(connexion);

          //suppresion de l'utilisateur dans le fichier dat
          if ((fd = open("utilisateurs.dat",O_RDWR,0664)) == -1)
          {
            perror("Erreur de fopen() dans Modification");
            exit(1);
          }      

          lseek(fd,(pos-1)*sizeof(UTILISATEUR), SEEK_SET);
          if ((read(fd,&u,sizeof(UTILISATEUR))) == -1) 
          {
            perror("Erreur de read(1) dans estPresent");
          }
          strcpy(u.nom,"-1");
          u.hash = hash("");
          lseek(fd,(pos-1)*sizeof(UTILISATEUR), SEEK_SET);
          write(fd,&u,sizeof(UTILISATEUR));

          if (close(fd))
          {
            perror("Erreur de fclose() dans Modification");
            exit(1);
          }

          strcpy(phraseRetour, "Client supprimé !\n");
          strcpy(okORko, "OK");
        }

        operation.sem_op = 1;
        if (semop(idSem, &operation, 1) == -1)
          perror("(SERVEUR) Erreur de semop");
        fprintf(stderr, "(SERVEUR %d) Libération du sémaphore 0\n", getpid());
      }
      
      m.type=m.expediteur;
      m.expediteur=getpid();
      m.requete=DELETE_USER;
      strcpy(m.data1,okORko);
      strcpy(m.texte,phraseRetour);
      if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
      {
        perror("(SERVEUR) Erreur de msgsnd");
        exit(1);
      }

      break;

    case NEW_PUB:
      fprintf(stderr, "(SERVEUR %d) Requete NEW_PUB reçue de %d\n", getpid(), m.expediteur);
      int fd;
      int signalpub = 0;
      //création du fichier pub si il n'est pas existant 
      if ((fd = open("publicites.dat", O_RDWR|O_CREAT|O_EXCL, 0664)) == -1)
      {
        signalpub = 1;
        if ((fd = open("publicites.dat", O_RDWR)) == -1){ 
        perror("Erreur de fopen() dans Modification");
        exit(1);
        }
      }
      //ecriture dans le fichier pub
      lseek(fd, 0, SEEK_END);
      pub.nbSecondes = atoi(m.data1);
      strcpy(pub.texte, m.texte);
      write(fd, &pub, sizeof(PUBLICITE));
      close(fd);
      if(signalpub == 0)//envoie du signal au processus pub si le fichier n'existait pas
        kill(tab->pidPublicite, SIGUSR1);
          
      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr, "Pid Serveur 1 : %d\n", tab->pidServeur1);
  fprintf(stderr, "Pid Serveur 2 : %d\n", tab->pidServeur2);
  fprintf(stderr, "Pid Publicite : %d\n", tab->pidPublicite);
  fprintf(stderr, "Pid Admin     : %d\n", tab->pidAdmin);
  for (int i = 0; i < 6; i++)
    fprintf(stderr, "%6d -%20s- %6d %6d %6d %6d %6d - %6d\n", tab->connexions[i].pidFenetre,
            tab->connexions[i].nom,
            tab->connexions[i].autres[0],
            tab->connexions[i].autres[1],
            tab->connexions[i].autres[2],
            tab->connexions[i].autres[3],
            tab->connexions[i].autres[4],
            tab->connexions[i].pidModification);
  fprintf(stderr, "\n");
}

void Handler(int sig)
{

  switch (sig)
  {
  case SIGINT://supprime ou deconnecte les différentes ressources liees au serveur lors d'un CTRL+C
    if (msgctl(idQ, IPC_RMID, NULL) == -1)
    {
      perror("Erreur de msgctl");
      exit(1);
    }
    if (tab->pidPublicite != 0)
    {
      kill(tab->pidPublicite, SIGTERM);
    }

    if (shmctl(idShm, IPC_RMID, NULL) == -1)
    {
      perror("Erreur de shmctl");
      exit(1);
    }
    if (semctl(idSem, 0, IPC_RMID) == -1)
    {
      perror("Erreur de shmctl");
      exit(1);
    }
    

    mysql_close(connexion);
    exit(0);
    break;

  case SIGCHLD://supprime la pid du fils zombi dans la table de processus et enleve son pid de la table des connexions
    int pidmodif = wait(NULL);
    printf("(SERVEUR) Suppression du fils zombi %d\n", pidmodif);
    for (int i = 0; i < 6; i++)
    {
      if (tab->connexions[i].pidModification == pidmodif)
      {
        tab->connexions[i].pidModification = 0;
        i = 6;
      }
    }
  }
}

void SendAddUser(MESSAGE sender)
{
  MESSAGE AddUsserMessage;
  //envoie un message à tous les clients lorsqu'un nouveau client se connecte
  for (int i = 0; i < 6; i++)
  {
    if (strcmp(tab->connexions[i].nom, "") != 0)
    {
      AddUsserMessage.type = tab->connexions[i].pidFenetre;
      AddUsserMessage.expediteur = 1;
      AddUsserMessage.requete = ADD_USER;
      strcpy(AddUsserMessage.data1, sender.data2);
      if (msgsnd(idQ, &AddUsserMessage, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("(Serveur) Erreur de msgsnd");
        exit(1);
      }
      if (kill(AddUsserMessage.type, SIGUSR1) == -1)
      {
        perror("Erreur de kill");
      }
    }
  }

  //envoie au nouveau client le nom des différents clients connectes
  for (int i = 0; i < 6; i++)
  {
    if (strcmp(tab->connexions[i].nom, "") != 0)
    {
      AddUsserMessage.type = sender.expediteur;
      AddUsserMessage.expediteur = 1;
      AddUsserMessage.requete = ADD_USER;
      strcpy(AddUsserMessage.data1, tab->connexions[i].nom);

      if (msgsnd(idQ, &AddUsserMessage, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("(Serveur) Erreur de msgsnd");
        exit(1);
      }

      if (kill(AddUsserMessage.type, SIGUSR1) == -1)
      {
        perror("Erreur de kill");
        exit(1);
      }
    }
  }
}

void SendRemoveUser(MESSAGE sender)
{
  //envoi un message pour prevenir les autres client lors du LOGOUT d'un client
  MESSAGE RemoveUsserMessage;
  for (int i = 0; i < 6; i++)
  {
    if (strcmp("", tab->connexions[i].nom) != 0)
    {
      RemoveUsserMessage.type = tab->connexions[i].pidFenetre;
      RemoveUsserMessage.expediteur = 1;
      RemoveUsserMessage.requete = REMOVE_USER;
      strcpy(RemoveUsserMessage.data1, sender.data1);

      if (msgsnd(idQ, &RemoveUsserMessage, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("ICI5(Serveur) Erreur de msgsnd");
        exit(1);
      }
      if (kill(RemoveUsserMessage.type, SIGUSR1) == -1)
      {
        perror("Erreur de kill");
        exit(1);
      }
    }
  }
}
