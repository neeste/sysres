/* uni.h - replacement for unistd for LCC compiler */

typedef unsigned int ssize_t;

int         access(char *, int);
int         close(int);
long        lseek(int, long, int);
int         open(char *, int, int);
ssize_t     read(int, void *, size_t);
ssize_t     write(int, void *, size_t);
