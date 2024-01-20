#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd;


void handlerSIG(int sig);

int main()
{
  char *pShm;
  int lecture = 0;
  MESSAGE mPUB;
  mPUB.type=1;
  mPUB.expediteur=getpid();
  mPUB.requete=UPDATE_PUB;
  // Armement des signaux
  PUBLICITE pub;
  struct sigaction A;
  A.sa_handler=handlerSIG;
  sigemptyset(&A.sa_mask);
  

  if (sigaction(SIGUSR1, &A, NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }
  // Masquage de SIGINT

  
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);
  
  int rc;

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if((idQ = msgget(CLE,0)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if ((idShm = shmget(CLE, 200*sizeof(char), 0)) == -1)
  {
    perror("Erreur de shmget");
    exit(1);
  }

  // Attachement à la mémoire partagée
  if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
    perror("Erreur de shmat");
    exit(1);
  }
  OPEN_PUB:

  if ((fd = open("publicites.dat",O_RDWR)) == -1)//ouvre le fichier pub, si pas cree le processus attend
  {
    if (errno == ENOENT){ 
      pause();
      goto OPEN_PUB;
    }
    perror("Erreur de open()");
    exit(1);
  }
  
  while(1)
  {
    //verification si le serveur est encore present
    if(kill(getppid(),0) == -1){
      if (close(fd))
      {
        perror("Erreur de close()");
        exit(1);
      }
      exit(1);

    }
    
    // Lecture d'une publicité dans le fichier
    if ((lecture = read(fd, &pub, sizeof(PUBLICITE))) == -1)
    {
      close(fd);
      perror("Erreur de read()");
      exit(1);
    }
    else if(lecture!=sizeof(PUBLICITE)){
      lseek(fd,0, SEEK_SET);
      if(read(fd,&pub,sizeof(PUBLICITE)) == -1){
        close(fd);
        perror("Erreur de read()");
        exit(1);
      }
    }

    
    // Ecriture en mémoire partagée
    strcpy(pShm, pub.texte);

    // Envoi d'une requete UPDATE_PUB au serveur
    if (msgsnd(idQ,&mPUB,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
      perror("ICI(publicite) Erreur de msgsnd");
      exit(1);
    }
    sleep(pub.nbSecondes);

  }
  
  if (close(fd))
  {
    perror("Erreur de close()");
    exit(1);
  }
}

void handlerSIG(int sig){//armement sinon comportement par défaut de sigusr1 ferme le processus 
  return;
}