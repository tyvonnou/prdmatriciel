#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

// Type Etat 
typedef enum {
    // Attente 
    STATE_WAIT,
    // Multiplication 
    STATE_MULT,
    // Affichage 
    STATE_PRINT
} Etat;
// Type pour matrice
typedef struct {
    size_t row, col;
    int **values;
} matrix;
// Structure pour le produit repris du TP4
typedef struct {
    Etat state;
    int *pending_mult;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    size_t it, nb_mult;
    struct {
        size_t row, col;
        char *addr;
    } m[2];
    matrix result;
} Product;
// Le prod
Product prod;

off_t fget_size(const int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) return -1;
    return st.st_size;
}


int read_next_int(char *buf, int *nb, int index)
{
    int i;

    if (sscanf(buf + index, "%d%n", nb, &i) > 0) {
        return index + i;
    }
    return -1;
}

// Initialisation de la matrice 
int init_matrice(int **matrice, int nbligne, int nbcolonne, int start, int *nb){
  // Romain comment stp
  size_t i, j;

  for(i = 0; i < nbligne; i++){
    for(j = 0; j < nbcolonne; j++){
           matrice[i][j] = nb[start];
           //printf("%d\n", matrice[i][j]);
           start++;
        }
   }


   return 0;
}

int pending_mult(Product *prod){
    size_t i;
    int nb = 0;
    for (i = 0; i < prod->nb_mult; i++) {
        nb += prod->pending_mult[i];
    }
    return nb;
}

int main(int argc, char **argv)
{
    int fd, sz, i, nb[20], j, k, l, start;
    size_t len;
    off_t pa_offset;
    char *addr;

    

 //initialisation des variable de la matrice

    int nbmult, nbligneA, nbcolonneA, nbligneB, nbcolonneB;

    if (argc < 2) {
        printf("Usage: %s filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[2], "%zd", &len) != 1) {
        printf("offset must be a int\n");
        exit(EXIT_FAILURE);
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) handle_error("open");
    if ((sz = fget_size(fd)) == -1) handle_error("fstat");

    /* Taille d'une page (alignement) */
    pa_offset = len & ~(sysconf(_SC_PAGE_SIZE) - 1);

    /*
        Création de l'espace mémoire
        mmap(void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset)
    */
    printf("Fichier mis en mémoire :\n");
    addr = mmap(NULL, len - pa_offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
    if (addr == MAP_FAILED) handle_error("mmap");
    printf("%s", addr);

    // Lecture du nombre du multiplication
    index = 0;
    if ((index = read_next_int(addr, &nb, index)) == -1 || nb <= 0) {
        printf_err_exit("Nombre de multiplication invalide");
    }
    prod.it = nb;

   // On initialise l'état de l'application
    prod.state = STATE_WAIT;

   // Initialisation du mutex et de la condition
    if ((errno = pthread_mutex_init(&prod.mutex, NULL)) != 0) {
        handle_error();
    }
    if ((errno = pthread_cond_init(&prod.cond, NULL)) != 0) {
        handle_error();
    }
   
   // Permet de rentrer les valeurs du fichier dans le tableau nb
   i = 0;
   j = 0;
   while ((i = read_next_int(addr, &nb[j], i)) != -1) {
     j++;
   }

    printf("Je fais le calcul...\n");
    for (i = 0; i < prod.it; i++) {
        for (j = 0; j < 2; j++) {
            // Obtention du nombre de ligne de la matrice j
            if ((index = read_next_int(addr, &nb, index)) == -1 || nb <= 0) {
                printf_err_exit("Nombre de ligne de la matrice ou nombre de multiplication invalide");
            }
            prod.m[j].row = nb;

            // Obtention du nombre de colonne de la matrice j
            if ((index = read_next_int(addr, &nb, index)) == -1 || nb <= 0) {
                printf_err_exit("Nombre de colonne de la matrice invalide");
            }
            prod.m[j].col = nb;;
        }

   int **m1;
   m1 = (int **) malloc(sizeof(int *)*20);
   for(i = 0; i < 20; i++){
     m1[i]=(int *) malloc(sizeof(int)*20);
   }
   int **m2;
   m2 = (int **) malloc(sizeof(int *)*20);
   for(i = 0; i < 20; i++){
     m2[i]=(int *) malloc(sizeof(int)*20);
   }
   int **m3;
   m3 = (int **) malloc(sizeof(int *)*20);
   for(i = 0; i < 20; i++){
     m3[i]=(int *) malloc(sizeof(int)*20);
   }


   nbligneA = nb[1];
   nbcolonneA = nb[2];
   nbligneB = nb[3];
   nbcolonneB = nb[4];


   start = 5*nb[0];
   init_matrice(m1, nb[1], nb[2], start, nb);
   start += nb[1]*nb[2];
   init_matrice(m2, nb[3], nb[4], start, nb);


   for(i = 0; i < nbligneA; i++) {
    for(j = 0; j < nbcolonneB; j++) {
        m3[i][j] = 0;
        printf("res[%d][%d] =\n", i, j);
        for (k = 0; k < nbcolonneB; k++) {
            m3[i][j] += m1[i][k] * m2[k][j];
            printf("\t+ %d * %d\n", m1[i][k], m2[k][j]);
        }
        printf("\t= %d\n", m3[i][j]);
    }
  }

  printf("Matrice RES : \n");
  for(i = 0; i < nbligneA; i++) {
   for(j = 0; j < nbcolonneB; j++) {
     printf("\t%d", m3[i][j]);

   }
   printf("\n");
  }


   free(m1);
   free(m2);
   free(m3);


  // Suppression de l'espace mémoire  
    munmap(addr, len - pa_offset);
    close(fd);
    return 0;
}
