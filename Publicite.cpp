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
#include "protocole.h" // contient la cle et la structure d'un message

int idQ=-1, idShm=-1;
int fd=-1, rd=-1;
char *pShm=NULL;;

int main()
{
  // Armement des signaux

  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());

  idQ=msgget(CLE, 0);
  if(idQ==-1){
    perror("Erreur msgget (Publicite)\n");
    exit(1);
  }

  printf("idQ : %d\n", idQ);


  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());

  idShm = shmget(CLE, 200, 0);  
  if(idShm == -1){
    perror("Erreur shmget (Publicite)");
    exit(1);
  }

  // Attachement à la mémoire partagée
  pShm=(char*) shmat(idShm, NULL, 0);
  if(pShm == (char*)-1){
    perror("Erreur shmat (Publicite)");
    exit(1);
  }

  // Ouverture du fichier de publicité
  fd=open("publicites.dat", O_RDONLY);
  if(fd == -1){
    perror("Erreur open publicites.dat (Publicite)");
    exit(1);
  }

  while(1)
  {
  	PUBLICITE pub;
    // Lecture d'une publicité dans le fichier
    rd=read(fd, &pub, sizeof(PUBLICITE));
    if(rd==sizeof(PUBLICITE)){
      // Ecriture en mémoire partagée
      strcpy(pShm, pub.texte);

      // Envoi d'une requete UPDATE_PUB au serveur
      MESSAGE m;
      m.type=1;
      m.expediteur=getpid();
      m.requete=UPDATE_PUB;

      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1){
        perror("Erreur msgsnd UPDATE_PUB (Publicite)");
        exit(1);
      }

      sleep(pub.nbSecondes);

    }
    else if(rd==0){
      lseek(fd, 0, SEEK_SET);
    }
    else{
      perror("Erreur read fichier (Publicite)");
      exit(1);
    }

    
  }
}

