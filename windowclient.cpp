#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <signal.h>


extern WindowClient *w;

#include "protocole.h"

int idQ=-1, idShm=-1;
#define TIME_OUT 120
int timeOut = TIME_OUT;
char * pShm=NULL;
TAB_CONNEXIONS* tab;


struct sigaction sa;
static int log=0;

void handlerSIGUSR1(int sig);
void handlerSIGALARM(int sig);
void handlerSIGUSR2(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    w=this;
    ::close(2);
    logoutOK();

    

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());

    idQ=msgget(CLE, 0);
    if(idQ==-1){
      perror("Erreur msgget (Client)\n");
      exit(1);
    }

    printf("idQ : %d\n", idQ);


    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());

    idShm=shmget(CLE, 200, 0);
    if(idShm==-1){
      perror("Erreur shmget (Client)\n");
      exit(1);
    }

    printf("idShm : %d\n", idShm);

    // Attachement à la mémoire partagée (la file)
    pShm=(char*) shmat(idShm, NULL, 0);
    if(pShm==(char*)-1){
      perror("Erreur shmat (Client)\n");
      exit(1);
    }
    

    // Armement des signaux
    sa.sa_handler=handlerSIGUSR1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;

    if(sigaction(SIGUSR1, &sa, nullptr)==-1){
      perror("Erreur sigaction (Client)\n");
      exit(1);
    }

    sa.sa_handler=handlerSIGALARM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;

    if(sigaction(SIGALRM, &sa, nullptr)==-1){
      perror("Erreur sigaction (Client)\n");
      exit(1);
    }

    sa.sa_handler = handlerSIGUSR2;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
      perror("sigaction SIGUSR2 (Client)");
      exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    fprintf(stderr,"Envoi requête connect (Client)\n");

    MESSAGE m;
    m.type=1;
    m.expediteur=getpid(); 
    m.requete=CONNECT;

    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
      perror("Erreur de msgsnd connect (Client)");
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

    MESSAGE m;
    if(log){

      m.type=1;
      m.expediteur=getpid();
      m.requete=LOGOUT;

      fprintf(stderr,"Envoi requête logout (Client)\n");

      if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
        perror("Erreur de msgsnd logout (Client)\n");
        exit(1);
      }

      log=0;

    }

    m.type=1; 
    m.expediteur=getpid(); 
    m.requete=DECONNECT; 

    fprintf(stderr,"Envoi requête deconnect (Client)\n");

    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
      perror("Erreur msgsnd deconnect (Client)");
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

    MESSAGE m;

  m.type=1;
  m.expediteur=getpid();
  m.requete=LOGIN;

  if(isNouveauChecked()){
    strcpy(m.data1, "1");
  }else{
    strcpy(m.data1, "0");
  }


  strcpy(m.data2, getNom());
  strcpy(m.texte, getMotDePasse());


  fprintf(stderr,"Envoie requête login (Client)\n");

  if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
      perror("Erreur de msgsnd login (Client)\n");
      exit(1);
  }


}

void WindowClient::on_pushButtonLogout_clicked()
{
    // TO DO
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    if(!log) return;


    MESSAGE m;
    m.type=1;
    m.expediteur=getpid();
    m.requete=LOGOUT;

    fprintf(stderr,"Envoie requête logout (Client)\n");

    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
        perror("Erreur de msgsnd logout(Client)\n");
        exit(1);
    }

    log=0;

    
    logoutOK();
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
    // TO DO
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    m.type=1;
    m.expediteur=getpid();
    m.requete=SEND;
      
    strcpy(m.texte, getAEnvoyer()); 


    fprintf(stderr,"Envoie requête send (Client)\n");

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
      perror("Erreur msgsnd (Client)\n");
      exit(1);
    }
}

void WindowClient::on_pushButtonConsulter_clicked()
{
    // TO DO
  alarm(0);
  timeOut = TIME_OUT;
  w->setTimeOut(timeOut);
  alarm(1);

  MESSAGE m;
  m.type=1;
  m.expediteur=getpid();
  m.requete=CONSULT;

  strcpy(m.data1, getNomRenseignements());


  fprintf(stderr,"Envoie requête consult (Client)\n");

  if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
    perror("Erreur msgsnd (Client)\n");
    exit(1);
  }

}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO

  alarm(0);
  timeOut = TIME_OUT;
  w->setTimeOut(timeOut);
  alarm(1);

  // Envoi d'une requete MODIF1 au serveur
  MESSAGE m;
  // ...
  m.type=1;
  m.expediteur=getpid();
  m.requete=MODIF1;

  fprintf(stderr,"Envoie requête modif1(Client)\n");

  if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
    perror("Erreur msgsnd (Client)\n");
    exit(1);
  }


  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...
  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), getpid(), 0)==-1){
    perror("Erreur msgrcv(Client)\n");
    exit(1);
  }

  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
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
  // ...
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));

    const char *personne;
    personne=getPersonneConnectee(1);

    if(personne==NULL || strlen(personne)==0) return;

    m.type=1;
    m.expediteur=getpid();
    strcpy(m.data1, personne);


    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        // TO DO (etape 2)
        m.requete=ACCEPT_USER;
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        // TO DO (etape 2)
        m.requete=REFUSE_USER;

    }

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd (Client)\n");
        exit(1);
    }

}

void WindowClient::on_checkBox2_clicked(bool checked)
{
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));

    const char *personne;
    personne=getPersonneConnectee(2);

    if(personne==NULL || strlen(personne)==0) return;
    
    m.type=1;
    m.expediteur=getpid();
    strcpy(m.data1, personne);

    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        // TO DO (etape 2)
        m.requete=ACCEPT_USER;

    }
    else
    {
        ui->checkBox2->setText("Refusé");
        // TO DO (etape 2)
        m.requete=REFUSE_USER;

    }

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd (Client)\n");
        exit(1);
    }
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));

    const char *personne;
    personne=getPersonneConnectee(3);

    if(personne==NULL || strlen(personne)==0) return;
    
    m.type=1;
    m.expediteur=getpid();
    strcpy(m.data1, personne);

    if (checked)
    {
        ui->checkBox3->setText("Accepté");
        // TO DO (etape 2)
        m.requete=ACCEPT_USER;

    }
    else
    {
        ui->checkBox3->setText("Refusé");
        // TO DO (etape 2)
        m.requete=REFUSE_USER;

    }

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd (Client)\n");
        exit(1);
    }
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));

    const char *personne;
    personne=getPersonneConnectee(4);

    if(personne==NULL || strlen(personne)==0) return;
    
    m.type=1;
    m.expediteur=getpid();
    strcpy(m.data1, personne);

    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        m.requete=ACCEPT_USER;

        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        // TO DO (etape 2)
        m.requete=REFUSE_USER;

    }

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd (Client)\n");
        exit(1);
    }
}

void WindowClient::on_checkBox5_clicked(bool checked)
{

    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));

    const char *personne;
    personne=getPersonneConnectee(5);

    if(personne==NULL || strlen(personne)==0) return;
    
    m.type=1;
    m.expediteur=getpid();
    strcpy(m.data1, personne);

    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        
        // TO DO (etape 2)
        m.requete=ACCEPT_USER;

    }
    else
    {
        ui->checkBox5->setText("Refusé");
        // TO DO (etape 2)
        m.requete=REFUSE_USER;

    }

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd (Client)\n");
        exit(1);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    int i;
    const char *nom, *myname;

    int deja;


    // ...msgrcv(idQ,&m,...)
    while(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long),getpid(), IPC_NOWAIT)!=-1){
  
      switch(m.requete)
      {
        case LOGIN :
                    if (strcmp(m.data1,"OK") == 0)
                    {
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      w->dialogueMessage("Login...",m.texte);
                      // ...
                      log=1;
                      timeOut = TIME_OUT;
                      w->setTimeOut(timeOut);
                      alarm(1);
                    }
                    else{
                      fprintf(stderr,"(CLIENT %d) Login KO\n",getpid());
                      w->dialogueErreur("Login...",m.texte);
                      w->logoutOK();
                      log=0;

                    } 
                    break;

        case ADD_USER :
                    // TO DO
                    deja=0;
                    myname=w->getNom();
                    if(myname!=NULL && strcmp(m.data1, myname) == 0) break;


                    for(i=1; i<=5; i++){
                      nom=w->getPersonneConnectee(i);
                      if(nom!=NULL && strcmp(nom, m.data1)==0){
                        deja=1;
                        break;
                      }
                    }

                    if(!deja){
                      for(i=1; i<=5; i++){
                        nom=w->getPersonneConnectee(i);
                        if(nom==NULL || strlen(nom)==0){
                          w->setPersonneConnectee(i, m.data1);
                          break;
                        }
                      }
                      w->ajouteMessage(m.data1, "est connecté !");

                    }
                   


                    break;

        case REMOVE_USER :
                    // TO DO

                    for(i=1; i<=5; i++){
                        nom=w->getPersonneConnectee(i);
                        if(nom!=NULL && strcmp(nom, m.data1)==0){
                          w->setPersonneConnectee(i, "");
                          break;
                        }
                      }
                    w->ajouteMessage(m.data1, "est déconnecté !");

                    break;

        case SEND :
                    // TO DO

                    w->ajouteMessage(m.data1, m.texte);
               
                    break;



        case CONSULT :
                  // TO DO

                  if(strcmp(m.data1, "OK") == 0)
                  {
                    w->setGsm(m.data2);
                    w->setEmail(m.texte);
                  }
                  else
                  {
                    w->setGsm("Non trouvé");
                    w->setEmail("Non trouvé");
                  }
                  break;


    }

  }
}


void handlerSIGALARM(int sig){

  timeOut--;
  w->setTimeOut(timeOut);

  

  if(timeOut==0){
    MESSAGE m;
    m.type=1;
    m.expediteur=getpid();
    m.requete=LOGOUT;

    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
      perror("Erreur msgsnd handlerSIGALARM (Client)\n");
      exit(1);
    }

    w->logoutOK();

    alarm(0);
    timeOut = TIME_OUT;
    w->setTimeOut(timeOut);
    return;

  }

  alarm(1);
}

void handlerSIGUSR2(int sig){
  w->setPublicite(pShm);
}