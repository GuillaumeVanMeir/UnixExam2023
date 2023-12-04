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

#include "FichierUtilisateur.h"

int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;

void afficheTab();

MYSQL* connexion;
void Handler(int sig);

int main()
{
  // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
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

  // Creation des ressources
  fprintf(stderr, "(SERVEUR %d) Creation de la file de messages\n", getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }


  // Initialisation du tableau de connexions
  fprintf(stderr,"(SERVEUR %d) Initialisation de la table des connexions\n",getpid());
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }
  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite

  int i,k,j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());

    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }
    switch(m.requete)
    {
      case CONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      
                      for(int i=0;i<6;i++)
                      {
                        if(tab->connexions[i].pidFenetre==0)
                        {
                          tab->connexions[i].pidFenetre=m.expediteur;
                          i=6;
                        }
                      }

                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      printf("test: %d\n", m.expediteur);
                      for(int i = 0; i<6; i++){
                        if(tab->connexions[i].pidFenetre == m.expediteur){
                          tab->connexions[i].pidFenetre = 0;
                          i=6;
                        }
                      }
                      break; 

      case LOGIN :  
                      char phraseRetour[50],okORko[3];

                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);

                      if(strcmp(m.data1,"1")==0) //Nouveau Client
                      {
                        if(estPresent(m.data2)==0) //Nouveau Client coché + non existant OU FICHIER NON EXISTANT
                        {
                          ajouteUtilisateur(m.data2, m.texte);
                          strcpy(phraseRetour,"Nouveau client créé : bienvenue !\n");
                          strcpy(okORko,"OK");      
                        }
                        else //Nouveau Client coché + existant
                        {
                          strcpy(phraseRetour,"Client déjà existant !\n");
                          strcpy(okORko,"KO");  
                        }
                      }
                      else //NOUVEAU CLIENT NON COCHE
                      {

                        if(estPresent(m.data2)==0) //Nouveau Client non coché + non existant
                        {
                          strcpy(phraseRetour,"Client inconnu...\n");
                          strcpy(okORko,"KO");  
                        }
                        else //Nouveau Client non coché + existant
                        {
                          if(verifieMotDePasse(estPresent(m.data2),m.texte)==-1)
                          {
                            strcpy(phraseRetour,"Le fichier n'existe pas\n");
                            strcpy(okORko,"KO");  
                          }

                          if(verifieMotDePasse(estPresent(m.data2),m.texte)==1)
                          {
                            strcpy(phraseRetour,"Re-bonjour cher client !\n");
                            strcpy(okORko,"OK");  
                          }

                          if(verifieMotDePasse(estPresent(m.data2),m.texte)==0)
                          {
                            strcpy(phraseRetour,"Mot de passe incorrect...\n");
                            strcpy(okORko,"KO");  
                          }      
                        }
                      }

                      MESSAGE retourReponse;

                      retourReponse.type = m.expediteur;
                      retourReponse.expediteur = getpid();
                      retourReponse.requete = LOGIN;
                      strcpy(retourReponse.data1,okORko); 
                      strcpy(retourReponse.texte,phraseRetour);
                      
                      if (msgsnd(idQ,&retourReponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                      {
                        perror("(Serveur) Erreur de msgsnd");
                        exit(1);
                      }
                      
                      if (kill(m.expediteur, SIGUSR1) == -1)
                      {
                        perror("Erreur de kill");
                        exit(1);
                      }

                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case ACCEPT_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                      break;

      case REFUSE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
                      break;

      case SEND :  
                      fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                      break; 

      case UPDATE_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CONSULT :
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF1 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGIN_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGOUT_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      break;

      case DELETE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                      break;

      case NEW_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
}

void Handler(int sig)
{
  if(sig == 2){ 
    if (msgctl(idQ, IPC_RMID, NULL) == -1)
    {
      perror("Erreur de msgctl");
      exit(1);
    }
    mysql_close(connexion);
    exit(0);
  }
}
