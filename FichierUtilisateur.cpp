#include "FichierUtilisateur.h"
#include <sys/types.h>
#include <unistd.h> // read, write, lseek, close
#include <stdio.h> // perror
#include <fcntl.h> // open
#include <string.h>     // strlen, strncpy, strncmp

int estPresent(const char* nom)
{
  UTILISATEUR u;
  int fd, rc, pos=1;

  fd = open(FICHIER_UTILISATEURS,O_RDONLY);

  if(fd == -1) // si le fichier n'existe pas
  {
      perror("Erreur de open()");
      return -1;
  }
 

  while ((rc = read(fd, &u, sizeof(UTILISATEUR))) == sizeof(UTILISATEUR))
  {  
    if(strcmp(nom, u.nom)==0){
      close(fd);
      return pos;
    }
    pos++;
   
  }

  if (rc == -1) // si on n'a pas réussi à lire le fichier
  {
    perror("Erreur de read");
    close(fd);
    return -1;
  }
  
  close(fd);
  return 0; // fin du fichier, utilisateur pas trouv"*/

}

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  int somme=0, i=0;

   while (motDePasse[i]!='\0')
    {
        somme =somme+(int)motDePasse[i]*(i+1); // (int)motDePasse[i]= valeur ASCII du caractère
        i++;
    }

    somme=somme%97;
    return somme;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  int fd;

  UTILISATEUR u;
  int wr;

  strcpy(u.nom, nom);
  u.hash=hash(motDePasse);

  fd = open(FICHIER_UTILISATEURS,O_WRONLY | O_CREAT | O_APPEND, 0666); // 0644 = wr-r--r--
  if(fd == -1) 
  {
      perror("Erreur de open()");
      return;
  }

  wr=write(fd, &u, sizeof(UTILISATEUR));

  if (wr!= sizeof(UTILISATEUR)) { // on écrit la structure dans le fichier, si ça ne va pas alors
    perror("Erreur write");
    close(fd);
    return;
  }

  close(fd);

}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  int fd, ha, rc;
  off_t off;
  UTILISATEUR u;

  fd = open(FICHIER_UTILISATEURS,O_RDONLY);

  if(fd == -1)
  {
      perror("Erreur de open()");
      return -1;
  }  

  // pos = la position de l’utilisateur dans le fichier (1, 2, 3…)
  // (pos - 1) = combien d’enregistrements on veut “sauter” avant d’arriver au bon.
  // sizeof(UTILISATEUR) → combien d’octets prend un enregistrement.
  // On multiplie les deux pour avoir le décalage total en octets.

  off = (off_t)(pos - 1) * (off_t)sizeof(UTILISATEUR);
  if (lseek(fd, off, SEEK_SET) == (off_t)-1) {
      perror("lseek");
      close(fd);
      return -1;
  }

  // lseek() attend un paramètre de type off_t, pas un int.
  // Alors on transforme les nombres en off_t avant de faire la multiplication (c’est ce qu’on appelle un cast en C).
  
  rc = read(fd, &u, sizeof(UTILISATEUR));
  if (rc == -1)
  {
      perror("Erreur read");
      close(fd);
      return -1;
  }

  if (rc == 0) // fin de fichier --> position invalide
  {
      close(fd);
      return -1;
  }
  close(fd);


  ha=hash(motDePasse);

  if (u.hash == ha)
      return 1;   // mot de passe correct
  else
      return 0;   // mot de passe incorrect

      
}

////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  int fd, rc, cnt=0;
  UTILISATEUR u;

  fd = open(FICHIER_UTILISATEURS,O_RDONLY);

  if(fd == -1)
  {
      perror("Erreur de open()");
      return -1;
  }  

  rc=read(fd, &u, sizeof(UTILISATEUR));
  while(rc==sizeof(UTILISATEUR)){ // lire utilisateur par utilisateur
    vecteur[cnt] = u; // copie la structure dans le tableau
    cnt++; // compter 1 utilisateur en plus
    rc = read(fd, &u, sizeof(UTILISATEUR));
  }

  if (rc == -1) {
    perror("Erreur read");
    close(fd);
    return -1; 
  }

  close(fd);

  return cnt; // retourne le nb d'utilisateurs lus

}
