#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "matrix.h"

int read_next_int(char *buf, int *nb, int index)
{
    int i;

    if (sscanf(buf + index, "%d%n", nb, &i) > 0) {
        return index + i;
    }
    return -1;
}

int next_line(char *buf, int index)
{
    while (buf[index] && buf[index++] != '\n')
        ;
    return buf[index] ? index : -index;
}

int write_buf_file(int fd, char *buf)
{
    ssize_t len = strlen(buf);
    if (write(fd, buf, len) != len) {
        return -1;
    }
    return 0;
}

off_t get_size_from_fd(const int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) return -1;
    return st.st_size;
}

int * init_tab(size_t len, int d)
{
    int *tab;
    size_t i;
    if ((tab = (int *) malloc(sizeof(int) * len)) == NULL) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        tab[i] = d;
    }
    return tab;
}

int * init_tab_increment(size_t len, int start)
{
    int *tab;
    size_t i;
    if ((tab = (int *) malloc(sizeof(int) * len)) == NULL) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        tab[i] = start++;
    }
    return tab;
}

int matrix_malloc_values(matrix *m)
{
    size_t i, j;
    if ((m->values = (int **) malloc(sizeof(int *) * m->row)) == NULL) {
        return -1;
    }
    for (i = 0; i < m->row; i++) {
        if ((m->values[i] = (int *) malloc(sizeof(int) * m->col)) == NULL) {
            for (j = 0; j < i; j++) {
                free(m->values[i]);
            }
            free(m->values);
            return -1;
        }
    }
    return 0;
}

void matrix_free_values(matrix *m)
{
    size_t i;
    for (i = 0; i < m->row; i++) {
        free(m->values[i]);
    }
    free(m->values);
}

int matrix_prod(matrix *m1, matrix *m2, matrix *s)
{
    size_t i, j, k;

    if (m1 == NULL || m2 == NULL || s == NULL)
        return -1;

    s->row = m1->row;
    s->col = m2->col;
    if (matrix_malloc_values(s) == -1)
        return -1;

    for(i = 0; i < m1->row; i++) {
        for(j = 0; j < m2->col; j++) {
            s->values[i][j] = 0;
            // printf("res[%zu][%zu] = ", i, j);
            for (k = 0; k < s->col; k++) {
                s->values[i][j] += m1->values[i][k] * m2->values[k][j];
                // if (k == 0) {
                //     printf("(%d * %d) ", m1->values[i][k], m2->values[k][j]);
                // } else {
                //     printf("+ (%d * %d) ", m1->values[i][k], m2->values[k][j]);
                // }
            }
            // printf("= %d\n", s->values[i][j]);
        }
    }

    return 0;
}

int matrix_write(matrix *m, int fd)
{
    uint col;
    size_t i, j;
    char buf[BUF_MAX], tmp[20];

    col = m->col - 1;
    for (i = 0; i < m->row; i++) {
        strcpy(buf, "");
        for (j = 0; j < col; j++) {
            sprintf(tmp, "%d ", m->values[i][j]);
            strcat(buf, tmp);
        }
        sprintf(tmp, "%d\n", m->values[i][j]);
        strcat(buf, tmp);
        if (write_buf_file(fd, buf) == -1)
            return -1;
    }
    return 0;
}

void matrix_display(matrix *m)
{
    size_t i, j;
    for (i = 0; i < m->row; i++) {
        for (j = 0; j < m->col; j++) {
            printf("%d\t", m->values[i][j]);
        }
        putchar('\n');
    }
}
