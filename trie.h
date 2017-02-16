#ifndef TRIE_H
#define TRIE_H

#define R_SIZE 26

typedef struct trie {
	size_t count;
	struct trie **children;
} trie;

trie *trie_init(void);
void trie_insert(trie **, const char *, size_t);
size_t trie_get(trie *, const char *);

#endif
