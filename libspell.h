#ifndef LIBSPELL_H
#define LIBSPELL_H

/* Number of possible arrangements of a word of length ``n'' at edit distance 1 */
#define COMBINATIONS(n) n + n - 1 + 26 * n + 26 * (n + 1)

typedef struct set {
	char *a;
	char *b;
} set;

char ** spell(char *);
int is_known_word(char *);
void free_list(char **);

#endif
