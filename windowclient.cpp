#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
#include <unistd.h>

extern WindowClient *w;

#include "protocole.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>

int idQ, idShm;
#define TIME_OUT 120
int timeOut = TIME_OUT;
char *pshm;

void handlerSIG(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    ::close(2);
    logoutOK();

    MESSAGE MsgConnect;
    MsgConnect.type=1;
    MsgConnect.expediteur=getpid();
    MsgConnect.requete=CONNECT;
    
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());

    if ((idQ = msgget(CLE,0)) == -1)
    {
      fprintf(stderr,"\n(CLIENT %d) Erreur de msgget", getpid());
      exit(1);
    }

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    if ((idShm = shmget(CLE, 0, 0)) == -1)
    {
      perror("Erreur de shmget");
      exit(1);
    }

    // Attachement à la mémoire partagée
    if ((pshm = (char *)shmat(idShm, NULL, SHM_RDONLY)) == (char *)-1)
    {
      perror("Erreur de shmat");
      exit(1);
    }

    // Armement des signaux
    struct sigaction A;
    A.sa_handler=handlerSIG;
    sigemptyset(&A.sa_mask);
    // sigaddset(&A.sa_mask,SIGALRM);
    A.sa_flags = 0;

    if (sigaction(SIGUSR1,&A,NULL) == -1)
    {
      perror("Erreur de sigaction");
      exit(1);
    }
    if (sigaction(SIGALRM,&A,NULL) == -1)
    {
      perror("Erreur de sigaction");
      exit(1);
    }
    if (sigaction(SIGUSR2, &A, NULL) == -1)
    {
      perror("Erreur de sigaction");
      exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    
    if (msgsnd(idQ,&MsgConnect,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauChecked()
{
  if (ui->checkBoxNouveau->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveau->setEnabled(false);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->checkBoxNouveau->setEnabled(true);
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue /////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Clic sur la croix de la fenêtre ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
    // TO DO
    event=event;    

    MESSAGE MsgDeconnect;
    MsgDeconnect.type = 1;
    MsgDeconnect.expediteur = getpid();
    if(w->CLIENTLOGIN){
      // clic sur deconnecter
      MsgDeconnect.requete = LOGOUT;
      if (msgsnd(idQ, &MsgDeconnect, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
       perror("(Client) Erreur de msgsnd");
        exit(1);
      }
    }
    MsgDeconnect.requete = DECONNECT;
    if (msgsnd(idQ, &MsgDeconnect, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      // clic sur croix
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    

    QApplication::exit();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    // TO DO
    // message LOGIN (clic sur bouton Connecter)
    MESSAGE MsgLogin;
    MsgLogin.type = 1; //car envoi au serveur
    MsgLogin.expediteur = getpid();
    MsgLogin.requete = LOGIN;
    // Nom utilisateur = -1 qd utilisateur supprimé dans le fichier (pr mettre un flag du genre "tu peux réécrire ici" pr un prochain nouvel utilisateur)
    if(strcmp(getNom(),"-1")==0)
    {
      dialogueErreur("Login", "Nom d'utilisateur -1 réservé!!!");
      return;
    }
  
    // passage du Nom, mot de passe et de l'info nouveau client (1) ou non (0) 
    strcpy(MsgLogin.data2, getNom());
    strcpy(MsgLogin.texte, getMotDePasse());
    if(isNouveauChecked()==true)
    {
      strcpy(MsgLogin.data1, "1"); //nouveau client
    }
    else
    {
      strcpy(MsgLogin.data1, "0");
    }
    if (msgsnd(idQ, &MsgLogin, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }

}

void WindowClient::on_pushButtonLogout_clicked()
{
  //message LOGOUT (clic sur bouton Déconnecter)
    alarm(0);
    MESSAGE MsgLogout;

    MsgLogout.type=1;
    MsgLogout.expediteur = getpid();
    MsgLogout.requete = LOGOUT;

    if (msgsnd(idQ, &MsgLogout, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }

    logoutOK();
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    w->CLIENTLOGIN = 0;
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
    // TO DO
    // Desactiver timer
    alarm(0);

    // message SEND (clic sur bouton envoyer pour un message)
    MESSAGE MsgSendM;

    MsgSendM.type=1;
    MsgSendM.expediteur = getpid();
    MsgSendM.requete = SEND;
    strcpy(MsgSendM.texte, w->getAEnvoyer());

    // si texte du message != vide
    if(strcmp(MsgSendM.texte,"")!=0)
    {
      if (msgsnd(idQ, &MsgSendM, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("(Client) Erreur de msgsnd");
        exit(1);
      }

      // afficher son message dans sa zone de discussion
      w->ajouteMessage(getNom(),MsgSendM.texte);

      // reset zone saisie du message
      w->setAEnvoyer("");
    }
    
    // Activation timer
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_pushButtonConsulter_clicked()
{
    // TO DO
    // Desactiver timer
    alarm(0);
    
    // message CONSULT (Clic sur le bouton consulter qui envoie juste le nom saisi et est utile pour afficher son gsm et son mail)
    MESSAGE Msgconsult;

    Msgconsult.type = 1;
    Msgconsult.expediteur = getpid();
    Msgconsult.requete = CONSULT;
    strcpy(Msgconsult.data1, w->getNomRenseignements());
    if (strcmp(w->getNomRenseignements(),"")!=0){ 
      if (msgsnd(idQ, &Msgconsult, sizeof(MESSAGE) - sizeof(long), 0) == -1)
      {
        perror("(Client) Erreur de msgsnd");
        exit(1);
      }
    }
    w->setGsm("...en attente...");
    w->setEmail("...en attente...");

    // Activation timer
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  // Desactiver timer
  alarm(0);
  // Envoi d'une requete MODIF1 au serveur (clic sur bouton modifier)
  MESSAGE m;
  m.requete = MODIF1;
  m.expediteur = getpid();
  m.type = 1;
  if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    perror("(Client) Erreur de msgsnd");
    exit(1);
  }


  // Entre temps, le serveur envoie une requete modifi1 au processus modification avec le nom du client qui veut se modifier


  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());

  RCV_CLIENT:
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
  {
    if (errno == EINTR) // si msgrcv interrompu par un signal
      goto RCV_CLIENT;
    perror("(CLIENT) Erreur de msgrcv");
    exit(1);
  }


  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
    return;
  }

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),"",m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  // Envoi d'une requete MODIF2 au serveur (clic sur bouton ok de la fenetre modification (donc pour valiider les nouvelles données mdp, gsm et mail))
  strcpy(m.data1, motDePasse);
  strcpy(m.data2, gsm);
  strcpy(m.texte, email);
  m.requete = MODIF2;
  m.type = 1;
  m.expediteur = getpid();
  if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    perror("(Client) Erreur de msgsnd");
    exit(1);
  }

// Activation timer
  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
  // Desactiver timer
    alarm(0);

    // Envoi d'une requete ACCEPT_USER OU REFUSE_USER au serveur (coché OU non coché)
    MESSAGE MsgConvers;
    MsgConvers.type = 1; //car envoi au serveur
    MsgConvers.expediteur = getpid();
    strcpy(MsgConvers.data1, w->getPersonneConnectee(1) );


    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        // TO DO (etape 2)

        MsgConvers.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        // TO DO (etape 2)
        
        MsgConvers.requete = REFUSE_USER;
    }

    if (msgsnd(idQ, &MsgConvers, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    
    // Activation timer
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
    alarm(0);

    // Envoi d'une requete ACCEPT_USER OU REFUSE_USER au serveur (coché OU non coché)
    MESSAGE MsgConvers;
    MsgConvers.type = 1; //car envoi au serveur
    MsgConvers.expediteur = getpid();
    strcpy(MsgConvers.data1, w->getPersonneConnectee(2) );

    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        // TO DO (etape 2)

        MsgConvers.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox2->setText("Refusé");
        // TO DO (etape 2)
        
        MsgConvers.requete = REFUSE_USER;
    }

    if (msgsnd(idQ, &MsgConvers, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    alarm(0);

    // Envoi d'une requete ACCEPT_USER OU REFUSE_USER au serveur (coché OU non coché)
    MESSAGE MsgConvers;
    MsgConvers.type = 1; //car envoi au serveur
    MsgConvers.expediteur = getpid();
    strcpy(MsgConvers.data1, w->getPersonneConnectee(3) );

    if (checked)
    {
        ui->checkBox3->setText("Accepté");
        // TO DO (etape 2)

        MsgConvers.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox3->setText("Refusé");
        // TO DO (etape 2)
        
        MsgConvers.requete = REFUSE_USER;
    }

    if (msgsnd(idQ, &MsgConvers, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    alarm(0);

    // Envoi d'une requete ACCEPT_USER OU REFUSE_USER au serveur (coché OU non coché)
    MESSAGE MsgConvers;
    MsgConvers.type = 1; //car envoi au serveur
    MsgConvers.expediteur = getpid();
    strcpy(MsgConvers.data1, w->getPersonneConnectee(4) );

    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        // TO DO (etape 2)

        MsgConvers.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        // TO DO (etape 2)
        
        MsgConvers.requete = REFUSE_USER;
    }

    if (msgsnd(idQ, &MsgConvers, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
    alarm(0);

    // Envoi d'une requete ACCEPT_USER OU REFUSE_USER au serveur (coché OU non coché)
    MESSAGE MsgConvers;
    MsgConvers.type = 1; //car envoi au serveur
    MsgConvers.expediteur = getpid();
    strcpy(MsgConvers.data1, w->getPersonneConnectee(5) );

    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        // TO DO (etape 2)

        MsgConvers.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        // TO DO (etape 2)
        
        MsgConvers.requete = REFUSE_USER;
    }

    if (msgsnd(idQ, &MsgConvers, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      perror("(Client) Erreur de msgsnd");
      exit(1);
    }
    
    timeOut = TIME_OUT;
    setTimeOut(timeOut);
    alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIG(int sig)
{
  MESSAGE m;
  

  switch(sig)
  {
    case SIGUSR1 :        
      while(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),IPC_NOWAIT) != -1)       
      {         
        switch(m.requete)
        {           
          case LOGIN :
            if (strcmp(m.data1,"OK") == 0)                       
            { 
              fprintf(stderr,"(CLIENT %d) Login OK\n",getpid()); 
              w->loginOK();
              w->dialogueMessage("Login...",m.texte);
              timeOut = TIME_OUT;
              alarm(1);
              w->CLIENTLOGIN = 1;
            }

            else if (strcmp(m.data1, "KO") == 0)
              w->dialogueErreur("Login...",m.texte);

            break;

          case ADD_USER ://tout le monde est prévenu qu'il y a un nouvel utilisateur co
            for(int i=1; i<6; i++)
            {                         
              if(!strcmp(w->getPersonneConnectee(i),""))
              {                           
                w->setPersonneConnectee(i,m.data1);                           
                i=6;
                w->ajouteMessage(m.data1, " est connecté");
              }
              
            }

            break;

          case REMOVE_USER ://tout le monde est prévenu qu'il y a un nouvel utilisateur deco         
            // TO DO
            for(int i=1;i<6;i++)
            {                         
              if(strcmp(w->getPersonneConnectee(i),m.data1)==0)                         
              {             
                w->ajouteMessage(m.data1," est déconnecté");
                w->setPersonneConnectee(i,"");                           
                i=6;                         
              }

            }

            break;

          case SEND ://envoie d'un message            
            // TO DO
            w->ajouteMessage(m.data1,m.texte);
            break;

          case CONSULT ://consulter les infos mail et gsm d'un utilisateur
            // TO DO    
            if(strcmp(m.data1,"KO") == 0){
              w->setGsm("");
              w->setEmail("");
              w->dialogueErreur("Consult...", "NON TROUVE");
            }
            else if (strcmp(m.data1, "OK") == 0){
              w->setGsm(m.data2);
              w->setEmail(m.texte);
            }
            break;         
        } 
      }      
      break;

    case  SIGALRM:// gestion du timer
    {
      if (timeOut == 0)
      {
        w->on_pushButtonLogout_clicked();
      }
      else
      {
        timeOut--;
        w->setTimeOut(timeOut);
        alarm(1);
      }
      break;}

  case SIGUSR2://recup la publicité dans la mémoire partagée
    w->setPublicite(pshm);
    break;

  }
}
     
      