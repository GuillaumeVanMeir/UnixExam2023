#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <mysql.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "protocole.h"

int idQ,idSem;

int main()
{
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if((idQ = msgget(CLE,0)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  if ((idSem = semget(CLE, 0, 0)) == -1)
  {
    perror("(SERVEUR) Erreur de semget");
    exit(1);
  }


  MESSAGE m;
  // Lecture de la requête CONSULT
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());

  if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(SERVEUR) Erreur de msgrcv");
    exit(1);
  }

  // Tentative de prise bloquante du semaphore 0
  struct sembuf operation;
  operation.sem_num = 0;
  operation.sem_op = -1;
  operation.sem_flg = SEM_UNDO;

  if (semop(idSem, &operation, 1) == -1)
    perror("(MODIFICATION) Erreur de semop" );

  fprintf(stderr,"(CONSULTATION %d) Prise bloquante du sémaphore 0\n",getpid());

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(CONSULTATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];

  sprintf(requete,"SELECT * FROM UNIX_FINAL WHERE UPPER(Nom) LIKE UPPER('%s');",m.data1);
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);
  if ((tuple = mysql_fetch_row(resultat)) != NULL)
  {
    strcpy(m.data1,"OK");
    strcpy(m.data2,tuple[2]);
    strcpy(m.texte,tuple[3]);
  }
  else if((tuple = mysql_fetch_row(resultat)) == NULL)
  {
    strcpy(m.data1,"KO");
    strcpy(m.data2,"");
    strcpy(m.texte,"");
  }

  // Construction et envoi de la reponse
  m.type = m.expediteur;
  m.expediteur = getpid();
  if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
    perror("ICI1(Serveur) Erreur de msgsnd");
    exit(1);
  }
  if (kill(m.type, SIGUSR1) == -1)
  {
    perror("Erreur de kill");
    exit(1);
  }


  // Deconnexion BD
  mysql_close(connexion);

  operation.sem_op = 1;
  if (semop(idSem, &operation, 1) == -1)
    perror("(CONSULTATION) Erreur de semop");
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());

  exit(0);
}