#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sched.h>
#include <utmpx.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>


// TODO : créer les différents threads de calcul
/*
Le problème est qu'il faut passer en argument plein de valeurs

Donc possible solution faire une struct de matrice et passer cette struct en param
de create_pthread
*/



void wasteTime(unsigned long ms)
{
	unsigned long t,t0;
	struct timeval tv;
	gettimeofday(&tv,(struct timezone *)0);
	t0=tv.tv_sec*1000LU+tv.tv_usec/1000LU;
	do
	{
		gettimeofday(&tv,(struct timezone *)0);
		t=tv.tv_sec*1000LU+tv.tv_usec/1000LU;
	} while(t-t0<ms);
}


#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

off_t fget_size(const int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) return -1;
    return st.st_size;
}

/*****************************************************************************/
void * mult(void * nbligne, void * nbcolonne, void * maxIte,int **m1,int **m2,int **m3)
{
	size_t i, j, max,k;
	size_t iter;

	/*=>Recuperation de l'index, c'est a dire index = ... */
  i = (size_t)nbligne;
	j = (size_t)nbcolonne;
	max = (size_t)maxIte;

	/*Affectation d'un CPU:*/
	int nbcpu;
	int cpu;
	int idxOp = i + 1;
	cpu_set_t cpuset;

	nbcpu = sysconf(_SC_NPROCESSORS_ONLN);
	cpu = idxOp%nbcpu;

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);


	if( (sched_setaffinity(0, sizeof(cpuset), &cpuset)) == -1){
		perror("sched_setaffinity");
	}



	fprintf(stderr,"Begin mult m[%zu][%zu]\n",i,j);


		fprintf(stderr,"--> mult m[%zu][%zu]\n",i,j); /* La multiplication peut commencer */

		/*=>Effectuer la multiplication a l'index du thread courant... */

    for (k = 0; k < max; k++) {
      m3[i][j] += m1[i][k] * m2[k][j];
    //  printf("\t+ %d * %d\n", m1[i][k], m2[k][j]);           /* Affichage du calcul*/

    }
		wasteTime(200+(rand()%200)); /* Perte du temps avec wasteTime() */


    printf("\t= %d\n", m3[i][j]);


	fprintf(stderr,"Quit mult m[%zu][%zu]\n\n",i,j);
	return(nbligne);
}

/*****************************************************************************/


int read_next_int(char *buf, int *nb, int index)
{
    int i;

    if (sscanf(buf + index, "%d%n", nb, &i) > 0) {
        return index + i;
    }
    return -1;
}


int init_matrice(int **matrice, int nbligne, int nbcolonne, int start, int *nb){

  size_t i, j;

  for(i = 0; i < nbligne; i++){
    for(j = 0; j < nbcolonne; j++){
           matrice[i][j] = nb[start];
           printf("%d\n", matrice[i][j]);
           start++;
        }
   }


   return 0;
}


int main(int argc, char **argv)
{
    int fd, sz, i, nb[20], j, k, start;
    size_t len;
    off_t pa_offset;
    char *addr;
  	pthread_t *multTh;

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


    /* Allocation dynamique du tableau pour les threads multiplieurs */


   i = 0;
   j = 0;
   while ((i = read_next_int(addr, &nb[j], i)) != -1) {
     j++;
   }


  // Allocation des matrices

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


   // init matrice satr commence a 5 la 5ème valeur = debut de matrice 1
   start = 5*nb[0];
   init_matrice(m1, nb[1], nb[2], start, nb);
   start += nb[1]*nb[2]; // ici on calcul la taille de m1 pour init start a la bonne valeur
   init_matrice(m2, nb[3], nb[4], start, nb);

   //création de threads (ce qui est en commentaire sont les arguments a passer aux threads)
   multTh=(pthread_t *)malloc(nbcolonneB*sizeof(pthread_t));
   for(i = 0; i < nbligneA; i++) {
    for(j = 0; j < nbcolonneB; j++) {
        m3[i][j] = 0;
        pthread_create(&multTh[j], NULL, mult, /*(void*)i, (void*)j, (void *) nbcolonneB, m1, m2, m3*/);
        /*description des params :
        i = nbligne actuel
        j = nbcolonne actuel
        bon les autre sont assez explicit*/



    }
  }

// attente des threads
  for(i = 0; i < nbligneA; i++) {
   for(j = 0; j < nbcolonneB; j++) {
      pthread_join(multTh[i], NULL);
    }
}

//affichage res
printf("Matrice RES : \n");
for(i = 0; i < nbligneA; i++) {
 for(j = 0; j < nbcolonneB; j++) {
   printf("\t%d\n", m3[i][j]);

 }
 printf("\n");
}
    /*
        Suppression de l'espace mémoire
        munmap(void *__addr, size_t __len)
    */
    free(m1);
    free(m2);
    free(m3);
    printf("%s", addr);
    munmap(addr, len - pa_offset);
    close(fd);
    free(multTh);
    exit(EXIT_SUCCESS);
}