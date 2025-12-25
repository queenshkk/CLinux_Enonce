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

int idQ=-1,idSem=-1;

int main()
{
  
  MESSAGE m;

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());

  idQ=msgget(CLE, 0);
  if(idQ==-1){
    perror("Erreur msgget (Consultation)");
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  idSem=semget(CLE, 1, 0);
  if(idSem==-1){
    perror("Erreur semget (Consultation)");
    exit(1);
  }

  // Lecture de la requête CONSULT
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  
  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), getpid(), 0)==-1){
    perror("Erreur msgrcv (Consultation)");
    exit(1);
  }

 
  // Tentative de prise bloquante du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Prise bloquante du sémaphore 0\n",getpid());

  struct sembuf op;
  op.sem_num=0;
  op.sem_op=-1;
  op.sem_flg=0;
  if(semop(idSem, &op, 1)==-1)
  {
    perror("Erreur semop (Consultation)");
    exit(1);
  }

  // Connexion à la base de donnée

  MYSQL * connexion = mysql_init(NULL);
  fprintf(stderr,"(CONSULTATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    op.sem_op=1;
    semop(idSem, &op, 1);
    exit(1); 
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  
  MYSQL_RES  *resultat=NULL;
  MYSQL_ROW  tuple = NULL;
  char requete[200];


  MESSAGE rep;
  rep.type=m.expediteur;
  rep.expediteur=getpid();
  rep.requete=CONSULT;

  // sprintf(requete,...);
  snprintf(requete, sizeof(requete),"SELECT gsm, email FROM UNIX_FINAL WHERE nom='%s';", m.data1);
    
  if(mysql_query(connexion,requete)!=0){
    fprintf(stderr,"Erreur mysql query consultation\n");
    strcpy(rep.data2, "KO");
    strcpy(rep.texte, "");
    strcpy(rep.data1, "");
  }
  else{
    resultat = mysql_store_result(connexion);
    MYSQL_ROW  tuple=NULL;
    if(resultat!=NULL && (tuple=mysql_fetch_row(resultat))!= NULL){
      strcpy(rep.data1, "OK");
      strcpy(rep.data2, tuple[0]);
      strcpy(rep.texte, tuple[1]);
      
    }
    else{
        strcpy(rep.data1, "KO");
        strcpy(rep.data2, "");
        strcpy(rep.texte, "");
    }
  }
  if(resultat) mysql_free_result(resultat);


    // if ((tuple = mysql_fetch_row(resultat)) != NULL) ...
  // Construction et envoi de la reponse

  if(msgsnd(idQ, &rep, sizeof(MESSAGE)-sizeof(long),0)==-1)
  {
    perror("Erreur msgsnd (Consultation)\n");
    op.sem_op=1;
    semop(idSem, &op, 1);
    exit(1);
  }
  kill(m.expediteur,SIGUSR1);

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());

  op.sem_op=1;
  if(semop(idSem, &op, 1)==-1){
    perror("Erreur semop V (Consultation)");
    exit(1);
  }


  exit(0);
}