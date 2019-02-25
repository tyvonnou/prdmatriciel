#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <sched.h>
#include <utmpx.h>
#include <sys/stat.h>

#define BUF_MAX     255
#define ANSI_RED "\x1b[31m"
#define ANSI_BOLD   "\x1b[1m"
#define ANSI_RESET  "\x1b[0m"
// Gestion de l'erreur 
#define printf_err_exit(msg) \
    do {\
        fprintf(stderr, ANSI_BOLD ANSI_RED "ERREUR" ANSI_RESET " - ""%s:%d - %s\n", __FILE__, __LINE__, msg); \
        exit(EXIT_FAILURE); \
    } while (0)
#define handle_error() printf_err_exit(strerror(errno))

// Les structures 
typedef unsigned int uint;
// la structure d'une matrice 
typedef struct {
    // Colonnes et ligne 
    size_t row, col;
    // Valeurs 
    int **values;
} matrix;

// Les différents états pour es threads 
typedef enum {
    // L'état d'attente 
    STATE_WAIT,
    // L'état de multiplication
    STATE_MULT,
    // L'état d'affichage
    STATE_PRINT
} State;

// La structure product
typedef struct {
    // Avec l'état 
    State state;
    // Liste des index des threads 
    int *pending_mult;
    // Les paramètres pour les threads 
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    // Le nombre de multiplication et le nombre d'itération
    size_t it, nb_mult;
    // Structure comme matrix avec un char * a la place d'un int ** 
    struct {
        size_t row, col;
        char *addr;
    } m[2];
    // Une matrice result
    matrix result;
} Product;

// Mon Product prod
Product prod;

// Fonction qui permet de récupérer une suite de caractère avec un sscanf et retourne le nouvel index 
int read_next_int(char *buf, int *nb, int index)
{   // Déclaration de l'index
    int i;
    // Lecture du caractère et de la position 
    if (sscanf(buf + index, "%d%n", nb, &i) > 0) {
        // Retourne le prochain index
        return index + i;
    }
    // Retourne -1 en cas d'erreur 
    return -1;
}

// Fonction qui écrit une chaîne dans un fichier 
int write_buf_file(int fd, char *buf)
{
    // On récupère la taille 
    ssize_t len = strlen(buf);
    // On écrit 
    if (write(fd, buf, len) != len) {
        // Retourne -1 si problème
        return -1;
    }
    // Sinon retourne 0
    return 0;
}
 
// Fonction qui récupère la taille à partir d'un id de fichier
off_t get_size_from_fd(const int fd)
{	
    struct stat st;
    // Demande les stat d'un programme 
    if (fstat(fd, &st) == -1) return -1;
    // Retourne la taille du fichier 
    return st.st_size;
}

//Fonction pour créer un tableau, retourne le tableau
int * init_tab(size_t len, int d)
{   
    // Déclaration d'un pointeur
    int *tab;
    size_t i;
    // Si le malloc ne marche pas je retourne null sinon je continue
    if ((tab = (int *) malloc(sizeof(int) * len)) == NULL) {
        return NULL;
    }
    // Rentre la valeur d par défaut dans toutes les cases 
    for (i = 0; i < len; i++) {
        tab[i] = d;
    }
    // Puis je retourne tab 
    return tab;
}

// Allocation des valeurs des matrices 
int matrix_malloc_values(matrix *m)
{
    size_t i, j;
    // J'essaye d'allouer de la mémoire , si ça marche je continue sinon je retourne -1
    if ((m->values = (int **) malloc(sizeof(int *) * m->row)) == NULL) {
        return -1;
    }
    // Pour chaques ligne
    for (i = 0; i < m->row; i++) {
	// J'essaye d'allouer de la mémoire 
        if ((m->values[i] = (int *) malloc(sizeof(int) * m->col)) == NULL) {
	    // Si ça ne marche pas je libère 
            for (j = 0; j < i; j++) {
                free(m->values[i]);
            }
            free(m->values);
	    // Et je retourne -1
            return -1;
        }
    }
    // Si tout ok je retourne 0
    return 0;
}

// Fonction qui libère les matrice 
void matrix_free_values(matrix *m)
{
    size_t i;
    // Pour chaques lignes 
    for (i = 0; i < m->row; i++) {
	// Je libère la mémoire 
        free(m->values[i]);
    }
    // Et je libère aussi le tableau de pointeurs 
    free(m->values);
}

// Fonction qui permet d'écrire  une matrice dans un fichier 
int matrix_write(matrix *m, int fd)
{
    uint col;
    size_t i, j;
    char buf[BUF_MAX], tmp[20];
    // Mes colonnes = colonnes -1
    col = m->col - 1;
    // Pour chaque ligne 
    for (i = 0; i < m->row; i++) {
        strcpy(buf, "");
	//Pour chaques colonnes 
        for (j = 0; j < col; j++) {
	    // Je rentre la valeur
            sprintf(tmp, "%d ", m->values[i][j]);
	    // et je l'écris 
            strcat(buf, tmp);
        }
	//Pour la dernière je met un \n a la place d'un espace
        sprintf(tmp, "%d\n", m->values[i][j]);
        strcat(buf, tmp);
	// J'écris ensuite, si ok je retourne 0 sinon -1
        if (write_buf_file(fd, buf) == -1)
            return -1;
    }
    return 0;
}

// Fonction pending mult qui permet d'indexer les threads 
int pending_mult(Product *prod){
    size_t i;
    int nb = 0;
    for (i = 0; i < prod->nb_mult; i++) {
        nb += prod->pending_mult[i];
    }
    // Je retourne nb, si nb = 0 tous les threads on fait leurs calculs 
    return nb;
}

void * mult(void *data)
{

    int  i, j, imatrix[2], values[2];
    // On récupére les données dans la variable data
    const int index = ((int *) data)[0], row = ((int *) data)[1], col = ((int *) data)[2];
    char *maddr[2];
    /*Affectation d'un CPU:*/
    int nbcpu;
    int cpu;
    int idxOp = index + 1;
    cpu_set_t cpuset;

    nbcpu = sysconf(_SC_NPROCESSORS_ONLN);
    cpu = idxOp%nbcpu;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if( (sched_setaffinity(0, sizeof(cpuset), &cpuset)) == -1){
        perror("sched_setaffinity");
    }
    // Affichage du CPU sur lequel on fait la multiplication
    fprintf(stderr,"Begin mult on proc %d\n", sched_getcpu());
    // On initialise l'état a attente
    pthread_mutex_lock(&prod.mutex);
    while (prod.state != STATE_MULT) {
        pthread_cond_wait(&prod.cond, &prod.mutex);
    }
    pthread_mutex_unlock(&prod.mutex);
	
    prod.result.values[row][col] = 0;
    // On cherche la bonne ligne de la matrice A
    imatrix[0] = 0;
    maddr[0] = prod.m[0].addr;
    for (i = 0; i < row; i++) {
        maddr[0] = strchr(maddr[0] + 1, '\n');
    }

    maddr[1] = prod.m[1].addr;
    for (i = 0; i < prod.m[0].col; i++) {
        imatrix[0] = read_next_int(maddr[0], values, imatrix[0]);
        // On cherche ensuite la bonne colonne 
        imatrix[1] = 0;
        for (j = 0; j < col; j++) {
            imatrix[1] = read_next_int(maddr[1], values + 1, imatrix[1]);
        }
        imatrix[1] = read_next_int(maddr[1], values + 1, imatrix[1]);
        maddr[1] = strchr(maddr[1] + 1, '\n');
        prod.result.values[row][col] += values[0] * values[1];
    }

    // Le thread déclare avoir fini son calcul
    pthread_mutex_lock(&prod.mutex);
    prod.pending_mult[index] = 0;
    // Le thread regarde si c'est le dernier, si c'est le cas je passe en affichage 
    if (pending_mult(&prod) == 0) {
        prod.state = STATE_PRINT;
        pthread_cond_broadcast(&prod.cond);
    }
    pthread_mutex_unlock(&prod.mutex);
    // Je retourne data
    return (data);
}

// Programme principal 
int main(int argc, char *argv[])
{
    int fd;
    char *addr, buf[BUF_MAX], filename[] = "resultatCPU.txt";
    off_t pagesize, page_offset, sz;
    size_t i, row, col;
    int nb, index, j;

    pthread_t *mult_th;
    int **mult_data;
//    struct timeval *t1, *t2;

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  //  gettimeofday(&t1);

    // Ouverture du fichier
    if ((fd = open(argv[1], O_RDONLY)) == -1)
        handle_error();
    if ((sz = get_size_from_fd(fd)) == -1) {
        close(fd);
        handle_error();
    }

    // Obtention de la taille d'une page mémoire
    pagesize = sysconf(_SC_PAGE_SIZE);

    // Calcul pour avoir le premier offset de la page mémoire
    page_offset = sz & ~(pagesize - 1);

    // Création du page mémoire privé et en lecture seulement
    addr = mmap(NULL, sz - page_offset, PROT_READ, MAP_PRIVATE, fd, page_offset);
    close(fd);
    if (addr == MAP_FAILED) {
        handle_error();
    }
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC)) == -1) {
        handle_error();
    }

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
    // Réalisation du calcul
    printf("Je fais le Calcul...\n");
    // Pour chaques itérations (nb de multiplication de matrice)
    for (i = 0; i < prod.it; i++) {
        // Boucle qui récupère nb ligne et nb colonnes 
        for (j = 0; j < 2; j++) {
            // Obtention du nombre de ligne de la matrice j
            if ((index = read_next_int(addr, &nb, index)) == -1 || nb <= 0) {
                printf_err_exit("Le nombre de ligne de la matrice ou nombre de multiplication n'est pas valide");
            }
            prod.m[j].row = nb;

            // Obtention du nombre de colonne de la matrice j
            if ((index = read_next_int(addr, &nb, index)) == -1 || nb <= 0) {
                printf_err_exit("Le nombre de colonne de la matrice n'est pas valide");
            }
            prod.m[j].col = nb;;
        }
        prod.m[0].addr = addr + index;
        for (j = 0; j < (prod.m[0].row * prod.m[0].col); j++) {
            index = read_next_int(addr, &nb, index);
        }
        prod.m[1].addr = addr + index;
        j = 0;
        // Décaler l'index pour prévenir la prochaine itération
        while (j++ < (prod.m[1].row * prod.m[1].col) && (index = read_next_int(addr, &nb, index)) != -1)
            ;

        // Préparation de la matrice de résultat (ligne + colonnes)
        prod.result.row = prod.m[0].row;
        prod.result.col = prod.m[1].col;

        // Allocation mémoire des valeurs de la matrice résultat, avec gestion d'erreur 
        if (matrix_malloc_values(&prod.result) == -1) {
            handle_error();
        }
        prod.nb_mult = prod.result.row * prod.result.col;

        // Initialisation du tableau pending_mult
        if ((prod.pending_mult = init_tab(prod.nb_mult, 1)) == NULL) {
            handle_error();
        }
        // Initialisation des tableaux données en argument aux threads
        if ((mult_data = (int **) malloc(sizeof(int *) * prod.nb_mult)) == NULL) {
            handle_error();
        }

        row = 0;
        col = 0;
        for (j = 0; j < prod.nb_mult; j++) {

            // On initialise chaque sous-tableau, sinon erreur 
            if ((mult_data[j] = (int *) malloc(sizeof(int) * 3)) == NULL) {
                handle_error();
            }
            if (col == prod.result.col) {
                col = 0;
                row++;
            }
            // mult_data est initialisé avec l'index du calcul, la ligne et la colonne de la matrice résultat 
            mult_data[j][0] = j;
            mult_data[j][1] = row;
            mult_data[j][2] = col++;
        }

        // Création des threads
        if ((mult_th = (pthread_t *) malloc(sizeof(pthread_t) * prod.nb_mult)) == NULL) {
            handle_error();
        }
        for (j = 0; j < prod.nb_mult; j++) {
            pthread_create(mult_th + j, NULL, mult, (void *) *(mult_data + j));
        }

        // Autoriser les multiplications
        pthread_mutex_lock(&prod.mutex);
        prod.state = STATE_MULT;
        pthread_cond_broadcast(&prod.cond);
        pthread_mutex_unlock(&prod.mutex);

        // Attendre l'autorisation de l'affichage
        pthread_mutex_lock(&prod.mutex);
        while (prod.state != STATE_PRINT) {
            pthread_cond_wait(&prod.cond, &prod.mutex);
        }
        pthread_mutex_unlock(&prod.mutex);

        // On écrit ensuite le résultat de l'itération courante
        sprintf(buf, "%zu %zu\n", prod.result.row, prod.result.col);
        nb = strlen(buf);
        if (write(fd, buf, nb) != nb) {
            handle_error();
        }
        if (matrix_write(&prod.result, fd) == -1) {
            handle_error();
        }

        // Libération de la mémoire
        for (j = 0; j < prod.nb_mult; j++) {
            free(mult_data[j]);
        }
        free(mult_data);
        matrix_free_values(&prod.result);
        free(mult_th);
        free(prod.pending_mult);
        free(mult_data);
    }

    printf("Calcul terminé résultat : %s\n", filename);
    // On ferme le fichier de résultat
    if (close(fd) == -1) {
        handle_error();
    }

    // Suppression du mutex et de la condition
    if ((errno = pthread_mutex_destroy(&prod.mutex)) != 0) {
        handle_error();
    }
    if ((errno = pthread_cond_destroy(&prod.cond)) != 0) {
        handle_error();
    }

    // Suppression de la page mémoire
    if (munmap(addr, sz - page_offset) == -1) {
        handle_error();
    }

    //gettimeofday(&t2);
//    printf("Temps de traitement : %ld µs\n", t2.tv_usec - t1.tv_usec);
    exit(EXIT_SUCCESS);
}