#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int idQ;


void handlerSIGUSR1(int sig);

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    idQ=msgget(CLE, 0);
    if(idQ==-1){
      perror("Erreur msgget (Client)\n");
      exit(1);
    }
    fprintf(stderr,"(ADMIN) idQ=%d\n", idQ);


    // Armement du signal
    struct sigaction sa;
    sa.sa_handler=handlerSIGUSR1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    if(sigaction(SIGUSR1, &sa, NULL)==-1){
        perror("Erreur sigaction (Admin)\n");
        exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    MESSAGE m;
    m.type = 1;               
    m.expediteur = getpid();
    m.requete = LOGIN_ADMIN;

    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0)==-1){ 
      perror("Erreur de msgsnd login (Client)\n");
      exit(1);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());
    if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), getpid(), 0)==-1){
        perror("Erreur msgrcv(Client)\n");
        exit(1);
    }

    if(strcmp(m.data1, "OK")==0)
    {
        QApplication a(argc, argv);
        WindowAdmin w;
        w.show();
        return a.exec();
    }
    
}

void handlerSIGUSR1(int sig){

}
