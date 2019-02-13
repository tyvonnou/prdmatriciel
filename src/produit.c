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



    /*
        Suppression de l'espace mémoire
        munmap(void *__addr, size_t __len)
    */
    printf("%s", addr);
    munmap(addr, len - pa_offset);
    close(fd);
    exit(EXIT_SUCCESS);
}
