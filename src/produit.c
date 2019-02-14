#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

off_t fget_size(const int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) return -1;
    return st.st_size;
}


int main(int argc, char **argv)
{
    int fd, sz;
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


    int pos;
    //TODO: Idée possible pour récupéré la position (%n)
    /*
    while (sscanf(s, "%d%n", &d, &pos) == 1) {
      arr[n++] = d;
      s += pos;
    } */
   sscanf( addr, "%d %d %d  %d %d", &nbmult, &nbligneA, &nbcolonneA, &nbligneB, &nbcolonneB);

   printf("%d %d %d  %d %d\n", nbmult, nbligneA, nbcolonneA, nbligneB, nbcolonneB);



   int i , j;
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

      for(i = 0; i < nbligneA; i++){
        for(j = 0; j < nbcolonneA; j++){
           sscanf(addr,"%d",&m1[i][j]);
           printf("%d",m1[i][j]);

        }
      }

         for(i = 0; i < nbligneB; i++){
           for(j = 0; j < nbcolonneB; j++){
              sscanf(addr,"%d",&m2[i][j]);
              printf("%d",m2[i][j]);

           }
         }

   for(i = 0; i < nbligneA; i++){
       for(j = 0; j < nbcolonneB; j++){
        m3[i][j] = m1[i][0]*m2[0][j]+m1[i][1]*m2[1][j];
        printf("%d\t",m3[i][j]);
       }
       printf("\n");
   }
   free(m1);
   free(m2);
   free(m3);



   return 0;

    /*
        Suppression de l'espace mémoire
        munmap(void *__addr, size_t __len)
    */
    printf("%s", addr);
    munmap(addr, len - pa_offset);
    close(fd);
    exit(EXIT_SUCCESS);
}
