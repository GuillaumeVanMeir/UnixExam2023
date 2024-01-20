#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int idQ;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
        fprintf(stderr, "\n(ADMINISTRATEUR %d) Erreur de msgget", getpid());
        exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    MESSAGE MsgConnect;

    MsgConnect.type = 1;
    MsgConnect.expediteur = getpid();
    MsgConnect.requete = LOGIN_ADMIN;

    if (msgsnd(idQ, &MsgConnect, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
        perror("(ADMINISTRATEUR) Erreur de msgsnd");
        exit(1);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());

    if (msgrcv(idQ, &MsgConnect, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
    {
        perror("(ADMINISTRATEUR) Erreur de msgrcv");
        exit(1);
    }
    if (strcmp(MsgConnect.data1, "KO") == 0)
    {
        printf("(ADMINISTRATEUR %d) Erreur: un autre admin est déjà connecté\n", getpid());
        exit(1);
    }
    else if (strcmp(MsgConnect.data1, "OK") == 0)
    {
        printf("(ADMINISTRATEUR %d) Connexion Réussi\n", getpid());
    }

    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
