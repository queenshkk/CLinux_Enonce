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
#include "FichierUtilisateur.h"

int idQ=-1,idSem=-1;

int main()
{
  MESSAGE m, rep;
  char nom[50];
  strcpy(nom, m.data1);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
  idQ=msgget(CLE, 0);
  if(idQ==-1){
    perror("Erreur msgget (Modification)");
    exit(1);
  }


  // Recuperation de l'identifiant du sémaphore
  idSem=semget(CLE, 1, 0);
  if(idSem==-1){
    perror("Erreur semget (Modification)");
    exit(1);
  }


  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());

  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), getpid(), 0)==-1){
    perror("Erreur msgrcv (Modification)");
    exit(1);
  }

  strcpy(nom, m.data1);
  rep.type=m.expediteur;
  rep.expediteur=getpid();
  rep.requete=MODIF1;

  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateut est déjà en train de modifier)
  struct sembuf op;
  op.sem_num=0;
  op.sem_op=-1;
  op.sem_flg=IPC_NOWAIT;

  if(semop(idSem, &op, 1)==-1)
  {
    strcpy(rep.data1, "KO");
    strcpy(rep.data2, "KO");
    strcpy(rep.texte, "KO");
 
    if(msgsnd(idQ, &rep, sizeof(MESSAGE)-sizeof(long),0)==-1)
    {
      perror("Erreur msgsnd (Modif)\n");
    }
    kill(m.expediteur, SIGUSR1);

    exit(0);
  }


  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
    op.sem_op = 1;
    semop(idSem, &op, 1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(),m.data1);
  
  MYSQL_RES *resultat = NULL;
  MYSQL_ROW tuple = NULL;
  char requete[200];

  snprintf(requete,sizeof(requete),"SELECT gsm, email FROM UNIX_FINAL WHERE nom='%s';", nom);

  if(mysql_query(connexion,requete)!=0){
    strcpy(rep.data1, "KO");
    strcpy(rep.data2, "KO");
    strcpy(rep.texte, "KO");
  }
  else{
    resultat = mysql_store_result(connexion);
    //tuple = mysql_fetch_row(resultat); // user existe forcement
    if(resultat!=NULL && (tuple=mysql_fetch_row(resultat))!= NULL){
        strcpy(rep.data1, "OK");
        strcpy(rep.data2, tuple[0]);
        strcpy(rep.texte, tuple[1]);
    }
    else{
        strcpy(rep.data1, "KO");
        strcpy(rep.data2, "KO");
        strcpy(rep.texte, "KO");
    }
  }
  
  if(resultat) mysql_free_result(resultat);


  // Construction et envoi de la reponse
  fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid()); 
  if(msgsnd(idQ,&rep,sizeof(MESSAGE)-sizeof(long),0)==-1){
    perror("Erreur msgsnd (Modification)\n");
    op.sem_op = 1;
    semop(idSem, &op, 1);
    exit(1);
  }
  
  kill(m.expediteur,SIGUSR1);

  // Attente de la requête MODIF2
  fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0)==-1){
    perror("Erreur msgrcv (Modification)");
    op.sem_op = 1;
    semop(idSem, &op, 1);
    exit(1);
  }
  
  
  // Mise à jour base de données
  fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),nom);
  //sprintf(requete,...);
  snprintf(requete,sizeof(requete),"UPDATE UNIX_FINAL SET gsm='%s', email='%s' WHERE nom='%s';",
         m.data2, m.texte, nom);
  
  mysql_query(connexion,requete);

 // Mise à jour du fichier si nouveau mot de passe

  if(strlen(m.data1)>0){

    int pos=estPresent(nom);
  
    if(pos>0){
      modifieMotDePasse(pos, m.data1);
      hash(m.data1);
    }
  }

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  op.sem_op=1;
  op.sem_flg = 0;
  if(semop(idSem, &op, 1)==-1){
    perror("Erreur semop V (Consultation)");
    exit(1);
  }

  exit(0);
}
