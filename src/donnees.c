#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(){
  // Nombre de multiplication, ainsi que index pour les boucles
  int NbMult, i,j,k;
  // Nombre de colonnes de la première matrice
  int nbColonnesA;
  // Nombre de lignes de la première matrice
  int nbLignesA;
  // Nombre de colonnes de la seconde matrice
  int nbColonnesB;
  // Nombre de lignes de la seconde matrice
  int nbLignesB;
  // Valeur (temporaire)
  int valeur;
  /* pointeur sur FILE */
  FILE *P_FICHIER;
  // Ouverture de fichier en écriture :
  P_FICHIER = fopen("matrice.txt", "w");
  // Récupération du nombre de multiplication à effectuer :
  printf("Nombre de multiplication à réaliser : ");
  scanf("%d", &NbMult);
  // écriture dans le fichier du nombre de multiplication :
  fprintf(P_FICHIER, "%d\n", NbMult);
  // Pour chaque oppérations
  for(i=0;i<(NbMult);i++){
    // Je demande le nombre de colonnes et de lignes de la première matrice
    printf("Matrice A Opération %d <nbLignes> <nbColonnes>: ", i+1);
    scanf("%d %d", &nbLignesA, &nbColonnesA);
    // Je les rentre dans le fichier
    fprintf(P_FICHIER, "%d %d\n", nbLignesA, nbColonnesA);
    // Même chose avec la deuxième matrice
    printf("Matrice B Opération %d <nbLignes> <nbColonnes>: ", i+1);
    scanf("%d %d", &nbLignesB, &nbColonnesB);
    fprintf(P_FICHIER, "%d %d\n", nbLignesB, nbColonnesB);
    printf("NomM[Colonne][Ligne]\n");
    // Pour chaque ligne de la première matrice
    for(j=0 ;j<nbLignesA ;j++){
      // Pour chaque colonnes de la première matrice
      for(k=0;k<nbColonnesA;k++){
        //je demande la valeur concerné
        printf("MatA[%d][%d] // Rentrez la valeur :", k,j);
        scanf("%d",&valeur);
        // Puis je la renseigne dans le fichier
        fprintf(P_FICHIER, "%d ", valeur);
      }
      // Je termine la ligne
      fprintf(P_FICHIER, "\n");
    }
    // Même chose avec la seconde matrice
    for(j=0;j<nbLignesB;j++){
      for(k=0;k<nbColonnesB;k++){
        printf("MatB[%d][%d] // Rentrez la valeur :", k,j);
        scanf("%d",&valeur);
        fprintf(P_FICHIER, "%d ", valeur);
      }
      fprintf(P_FICHIER, "\n");
    }
  }
  // Fermeture du fichier 
  fclose(P_FICHIER);
}
