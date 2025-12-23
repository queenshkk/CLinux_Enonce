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
int Login(MESSAGE *m);

MYSQL* connexion;
void HandlerSIGINT(int sig);


int main()
{
  struct sigaction A; 

  // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Armement des signaux
  A.sa_handler=HandlerSIGINT;
  A.sa_flags=0;
  sigemptyset(&A.sa_mask); 

  if(sigaction(SIGINT, &A, NULL)==-1){
    perror("Erreur sigaction (Serveur)\n");
    exit(1);
  }


  // Creation des ressources
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
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


  int fd = open(FICHIER_UTILISATEURS, O_CREAT | O_RDWR, 0666);
  if (fd == -1)
  {
      perror("Erreur creation utilisateurs.dat");
      exit(1);
  }
  close(fd);

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    printf("Requête : %d\n", m.requete);
    switch(m.requete)
    {
      case CONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);

                      for(i=0; i<6; i++){ 
                        if(tab->connexions[i].pidFenetre==0){
                          tab->connexions[i].pidFenetre=m.expediteur; 
                          break; 
                        }
                        
                      }
                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);

                       for(i=0; i<6; i++){ //
                        if(tab->connexions[i].pidFenetre==m.expediteur){
                          tab->connexions[i].pidFenetre=0; 
                          break; 
                        }
                      }

                      break; 

      case LOGIN :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);
                      int ok;
                      
                      ok=Login(&m);

                      if(ok){
                        for(i=0; i<6; i++){
                          if(tab->connexions[i].pidFenetre!=0 && tab->connexions[i].pidFenetre!=m.expediteur
                             && tab->connexions[i].nom[0]!='\0'){
                            MESSAGE autres;
                            autres.type=tab->connexions[i].pidFenetre;
                            autres.expediteur=getpid();
                            autres.requete=ADD_USER;
                            strcpy(autres.data1, m.data2);


                            if(msgsnd(idQ, &autres, sizeof(MESSAGE)-sizeof(long), 0)==-1){
                              perror("Erreur msgsnd (Serveur)");
                              exit(1);
                            }

                            kill(tab->connexions[i].pidFenetre, SIGUSR1);
                          }
                        }


                        for(i=0; i<6; i++){
                          if(tab->connexions[i].pidFenetre!=0 && tab->connexions[i].pidFenetre!=m.expediteur
                            && tab->connexions[i].nom[0]!='\0'){
                              MESSAGE autres;
                              autres.type=m.expediteur;
                              autres.expediteur=getpid();
                              autres.requete=ADD_USER;
                              strcpy(autres.data1, tab->connexions[i].nom);


                              if(msgsnd(idQ, &autres, sizeof(MESSAGE)-sizeof(long), 0)==-1){
                                perror("Erreur msgsnd (Serveur)");
                                exit(1);
                              }

                              kill(m.expediteur, SIGUSR1);
                            }
                        }

                      }
    
                      reponse.type=m.expediteur;
                      reponse.expediteur=getpid();
                      reponse.requete=LOGIN;
                      if (ok){
                        strcpy(reponse.data1, "OK");
                      }
                      else{
                        strcpy(reponse.data1, "KO");
                      }

                      strcpy(reponse.texte, m.texte);

                      if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
                        perror("Erreur de msgsnd login (Serveur)");
                        exit(1);
                      }

                      kill(m.expediteur, SIGUSR1);

                      
                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      
                      char nom[50];

                      for(i=0; i<6; i++){
                        if(tab->connexions[i].pidFenetre==m.expediteur){
                          strcpy(nom, tab->connexions[i].nom);

                          strcpy(tab->connexions[i].nom, "");
                          for(j=0; j<5; j++){
                            tab->connexions[i].autres[j]=0;
                          }
                          tab->connexions[i].pidModification=0;
                          break;
                        }
                      }

                      for(i=0;i<6; i++){
                        if(tab->connexions[i].pidFenetre<=0){
                          continue;
                        }
                        
                        MESSAGE rep;
                        rep.type=tab->connexions[i].pidFenetre;
                        rep.expediteur=getpid();
                        rep.requete=REMOVE_USER;

                        strcpy(rep.data1, nom);

                        if(msgsnd(idQ, &rep, sizeof(MESSAGE)-sizeof(long), 0)==-1){
                          perror("Erreur msgsnd remove user (Serveur)\n");
                          exit(1);
                        }

                        kill(tab->connexions[i].pidFenetre, SIGUSR1);
                      }
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

void HandlerSIGINT(int sig){

  if(idQ!=-1){
     if(msgctl(idQ, IPC_RMID, NULL)==-1){ 
      perror("Erreur msgctl (Serveur)\n");
      exit(1);
    }
  }
 
  if(connexion!=NULL){
    mysql_close(connexion);
    connexion=NULL;
  }

  exit(0);
}


int Login(MESSAGE *m){
  int pos, ok=0, verif;
  pos=estPresent(m->data2);

  if(pos>0){
    verif=verifieMotDePasse(pos, m->texte);
  }

  if(strcmp(m->data1, "1")==0){ // nouvel utilisateur
      if(pos>0){// position trouvée dans le fichier
        strcpy(m->texte, "Utilisateur déjà existant !");
        ok=0;
      }
      else if(pos==0){
        ajouteUtilisateur(m->data2, m->texte);
        strcpy(m->texte, "Nouvel utilisateur créé : bienvenue !");
        ok=1;
      }
      else{
        strcpy(m->texte, "Erreur accès fichier");
        ok=0;
      }
  }
  else{// ancien utilisateur
    if(pos==0){
      strcpy(m->texte, "Utilisateur inconnu...!");
      ok=0;
    }
    else if(pos>0){
      if(verif==1){ // mot de passe ok
      strcpy(m->texte, "Re-bonjour cher utilisateur!");
      ok=1;
      }
      else if(verif==0){ // mot de passe incorrect
        strcpy(m->texte, "Mot de passe incorrect...");
        ok=0;
      }
      else{
        strcpy(m->texte, "Erreur accès fichier");
        ok=0;
      }
    }
  }

  if(ok)
  {
    for (int i = 0; i < 6; i++)
    {
      if (tab->connexions[i].pidFenetre == m->expediteur)
      {
        strcpy(tab->connexions[i].nom, m->data2);
        break;
      }
    }
  }

  return ok;

}