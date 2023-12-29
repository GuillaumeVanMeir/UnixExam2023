#include "FichierUtilisateur.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int emplacementLibre()
{
  int fd,bytesLu,position=0;
  UTILISATEUR u;

  if ((fd = open(FICHIER_UTILISATEURS,O_RDONLY)) == -1)
  {
    perror("Erreur lors de l'ouverture du fichier");
    exit(1);
  }

  while((bytesLu = read(fd,&u,sizeof(UTILISATEUR)) ) == sizeof(UTILISATEUR))
  {
    position++;
    
    if(strcmp(u.nom,"-1") == 0)
    {
      if (close(fd))
      {
        perror("\nErreur de close() dans emplacementLibre");
        exit(1);
      }

      return position;
    }
  }

  if (close(fd))
  {
    perror("\nErreur de close() dans emplacementLibre");
    exit(1);
  }

  return 0;
}

int estPresent(const char* nom)
{
  // TO DO
  int rc,fd,position=0,nb,compteur=0;
  UTILISATEUR u;

  if ((fd = open(FICHIER_UTILISATEURS,O_RDWR|O_CREAT,0644)) != -1)
  {
    rc = lseek(fd,0,SEEK_END);
    if (rc == -1)
    {
      perror("Erreur de lseek(1) : ");
    }
    else
    {
      nb=rc/(sizeof(UTILISATEUR));
    }

    rc = lseek(fd,0,SEEK_SET);
    if (rc == -1)
    {
      perror("Erreur de lseek(2) : ");
    }
    else
    {
      while(nb!=0)
      {
        compteur++;
        nb--;
        if ((read(fd,&u,sizeof(UTILISATEUR))) == -1) //charge en mémoire les clients 1 à 1
        {
          perror("Erreur de read(1) dans estPresent");
        }

        if(strcmp(u.nom,nom)==0) //compare si les noms sont identiques
        {
          position=compteur;
          nb=0;
        }
      }

      if (close(fd))
      {
        perror("\nErreur de close() dans estPresent");
        exit(1);
      }
    }
  }
  else
  {
    perror("Erreur de open() dans estPresent");
  }

  return position;
}

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  // TO DO
  int taille=0,MDPhash=0,temp,position=0;

  taille=strlen(motDePasse);

  while(taille!=0)
  {
    temp=motDePasse[position];
    temp=temp*(position+1);
    MDPhash=temp+MDPhash;
    taille--;
    position++;
  }
  MDPhash=MDPhash%97;

  return MDPhash;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  // TO DO
  int fd,rc,mdpHash=0,retour=0;
  UTILISATEUR u;

    if ((fd = open(FICHIER_UTILISATEURS,O_RDWR | O_CREAT,0664)) == -1) //ouvre fichier en écriture 
    {
      perror("Erreur de open() dans ajouteClient");
    }

    retour=emplacementLibre();
    
    if(retour != 0)
    {
      lseek(fd,(retour-1)*sizeof(UTILISATEUR), SEEK_SET);

      if ((read(fd,&u,sizeof(UTILISATEUR))) == -1) 
      {
        perror("Erreur de read(1) dans ajouteClient");
      }
      strcpy(u.nom,nom);
      mdpHash=hash(motDePasse);
      u.hash=mdpHash;

      lseek(fd,(retour-1)*sizeof(UTILISATEUR), SEEK_SET);
      write(fd,&u,sizeof(UTILISATEUR));
    }
    else if(retour == 0)
    {
      if ((rc = lseek(fd,0,SEEK_END)) == -1) //position fin de fichier
      {
        perror("Erreur de lseek(3) dans ajouteClient");
        printf("Position dans le fichier : %d dans ajouteClient\n",rc);
      }
      else  //écris en fin de fichier le nom+hash du nouveau client
      {
        strcpy(u.nom,nom);
        mdpHash=hash(motDePasse);
        u.hash=mdpHash;

        write(fd,&u,sizeof(UTILISATEUR)); //Ecrit dans le fichier
      }
    }

    if (close(fd))
    {
      perror("\nErreur de close() dans ajouteClient");
      exit(1);
    }
  
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  // TO DO
  int fd,rc,mdpHash;
  UTILISATEUR u;

  pos--; //la position est 1,2,3... et pas 0,1,2,... donc on doit faire ça pour faire comme si on jouait avec des indices

  if ((fd = open(FICHIER_UTILISATEURS,O_RDONLY,0664)) == -1) //ouvre fichier en lecture
  {
    perror("Erreur de open() dans verifieMotDePasse");
    return -1;
  }

  if ((rc = lseek(fd,pos*sizeof(UTILISATEUR),SEEK_SET)) == -1) //position voulue dans le fichier
  {
    perror("Erreur de lseek(3) dans verifieMotDePasse");
    printf("Position dans le fichier : %d dans verifieMotDePasse\n",rc);
  }

  if ((rc = read(fd,&u,sizeof(UTILISATEUR))) == -1) //charge en mémoire le client
  {
    perror("Erreur de read(1) dans verifieMotDePasse");
  }

  mdpHash=hash(motDePasse);

  if(mdpHash==u.hash)
  {
    if (close(fd))
    {
      perror("\nErreur de close() dans verifieMotDePasse");
      exit(1);
    }
    return 1;
  }
  else
  {
    if (close(fd))
    {
      perror("\nErreur de close() dans verifieMotDePasse");
      exit(1);
    }
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  // TO DO
  int fd,rc,nb=0,i=0;
  UTILISATEUR u;

  if ((fd = open(FICHIER_UTILISATEURS,O_RDONLY,0664)) == -1) //ouvre fichier en lecture
  {
    perror("Erreur de open() dans listeClients");
    return -1;
  }
  else
  {
    if ((rc = lseek(fd,0,SEEK_END)) == -1) //position début de fichier
    {
      perror("Erreur de lseek(3) dans listeClients");
      printf("Position dans le fichier : %d dans listeClients\n",rc);
    }
    else
    {
      nb=rc/sizeof(UTILISATEUR);

      if ((rc = lseek(fd,0,SEEK_SET)) == -1) //position début de fichier
      {
        perror("Erreur de lseek(3) dans listeClients");
        printf("Position dans le fichier : %d dans listeClients\n",rc);
      }
      else
      {
        while(i<nb)
        {
          read(fd,&u,sizeof(UTILISATEUR));
          vecteur[i]=u;

          i++;
        }
      }
    }
    close (fd);
    
  }

  return nb;
}
