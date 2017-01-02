#ifndef LIBSPELL_H
#define LIBSPELL_H

#include <sys/rbtree.h>

/* Number of possible arrangements of a word of length ``n'' at edit distance 1 */
#define COMBINATIONS(n) n + n - 1 + 26 * n + 26 * (n + 1)

typedef struct set {
	char *a;
	char *b;
} set;

typedef struct word_count {
	char *word;
	size_t count;
	rb_node_t rbtree;
} word_count;

typedef struct spell_t {
	rb_tree_t *dictionary;
} spell_t;

char ** spell(char *);
int is_known_word(char *);
void free_list(char **);
spell_t *spell_init(FILE *);
int spell_is_known_word(spell_t *, char *);
char **spell_get_suggestions(spell_t *, char *, size_t);


#endif
