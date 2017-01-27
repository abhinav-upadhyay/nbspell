#ifndef LIBSPELL_H
#define LIBSPELL_H

#include <sys/rbtree.h>

/* Number of possible arrangements of a word of length ``n'' at edit distance 1 */
#define COMBINATIONS(n) n + n - 1 + 26 * n + 26 * (n + 1)

typedef struct word_count {
	char *word;
	size_t count;
	rb_node_t rbtree;
} word_count;

typedef struct spell_t {
	rb_tree_t *dictionary;
	rb_tree_t *ngrams_tree;
	rb_tree_t *whitelist;
	rb_tree_t *soundex_tree;
} spell_t;

void free_list(char **);
spell_t *spell_init(const char *, const char *);
int spell_is_known_word(spell_t *, const char *, int);
char **spell_get_suggestions(spell_t *, char *, int);
int is_whitelisted_word(spell_t *, const char *);
char *soundex(const char *);
void spell_destroy(spell_t *);
int compare_words(void *, const void *, const void *);
char *lower(char *);

#endif
