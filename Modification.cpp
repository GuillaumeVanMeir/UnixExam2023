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
#include <fcntl.h>
#include "protocole.h"
#include "FichierUtilisateur.h"

int idQ,idSem;

int main()
{
  char nom[40];

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
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
  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());

  if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(SERVEUR) Erreur de msgrcv");
    exit(1);
  }

  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateut est déjà en train de modifier)
  struct sembuf operation;
  operation.sem_num = 0;
  operation.sem_op = -1;
  operation.sem_flg = IPC_NOWAIT;
  if (semop(idSem, &operation, 1) == -1){
    strcpy(m.data1,"KO");
    strcpy(m.data2,"KO");
    strcpy(m.texte,"KO");
    m.type = m.expediteur;
    m.expediteur = getpid();
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Modification) Erreur de msgsnd");
      exit(1);
    }
  }
  else
  { 
    // Connexion à la base de donnée
    MYSQL *connexion = mysql_init(NULL);
    fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
    if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
    {
      fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
      exit(1);  
    }

    // Recherche des infos dans la base de données
    fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(),m.data1);
    strcpy(nom,m.data1);
    MYSQL_RES  *resultat;
    MYSQL_ROW  tuple;
    char requete[200];
    
    sprintf(requete,"SELECT * FROM UNIX_FINAL WHERE UPPER(Nom) LIKE UPPER('%s');",m.data1);
    mysql_query(connexion,requete);
    resultat = mysql_store_result(connexion);
    if ((tuple = mysql_fetch_row(resultat)) != NULL)
    {
      strcpy(m.data2,tuple[2]);
      strcpy(m.texte,tuple[3]);
    }
    else if((tuple = mysql_fetch_row(resultat)) == NULL)
    {
      strcpy(m.data1,"KO");
      strcpy(m.data2,"KO");
      strcpy(m.texte,"KO");
    }
    

    // Construction et envoi de la reponse
      fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());

    m.type = m.expediteur;
    m.expediteur = getpid();
    if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
      perror("(Modification) Erreur de msgsnd");
      exit(1);
    }

    
    // Attente de la requête MODIF2
    fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());

    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      exit(1);
    }

    // Mise à jour base de données
    fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),nom);

    if(strcmp(m.data2,"")!=0)
    {
      sprintf(requete,"UPDATE UNIX_FINAL SET gsm = '%s' WHERE UPPER(Nom) LIKE UPPER('%s');",m.data2,nom);
      mysql_query(connexion,requete);
    }

    if(strcmp(m.texte,"")!=0)
    {
      sprintf(requete,"UPDATE UNIX_FINAL SET email = '%s' WHERE UPPER(Nom) LIKE UPPER('%s');",m.texte,nom);
      mysql_query(connexion,requete);
    }

    // Mise à jour du fichier si nouveau mot de passe
    UTILISATEUR u;
    if(strcmp(m.data1,"")!=0)
    {
      int fd;

      if ((fd = open("utilisateurs.dat",O_RDWR,0664)) == -1)
      {
        perror("Erreur de fopen() dans Modification");
        exit(1);
      }      

      int boucle=1;
      while(boucle==1)
      {
        int i=0;
        if ((read(fd,&u,sizeof(UTILISATEUR))) == -1)
        {
          perror("Erreur de fread dans Modification");
          boucle=0;
          exit(1);
        }

        if(strcmp(u.nom,nom)==0)
        {
          lseek(fd,-sizeof(UTILISATEUR), SEEK_CUR);
          
          u.hash = hash(m.data1);
          write(fd,&u,sizeof(UTILISATEUR));
          boucle=0;
        }
      }

      if (close(fd))
      {
        perror("Erreur de fclose() dans Modification");
        exit(1);
      }
    }
    

    // Deconnexion BD
    mysql_close(connexion);
    // Libération du semaphore 0
    operation.sem_op = 1;
    if (semop(idSem, &operation, 1) == -1)
      perror("(MODIFICATION) Erreur de semop");
    fprintf(stderr,"(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  }


  exit(0);
}