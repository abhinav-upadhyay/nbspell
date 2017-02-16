#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "trie.h"

trie *
trie_init(void)
{
	trie *t = emalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->children = emalloc(R_SIZE * sizeof(*(t->children)));
	memset(t->children, 0, sizeof(*(t->children)));
	return t;
}

void
trie_insert(trie **t, const char *key, size_t value)
{
	const char *c = key;
	trie *cur_node = *t;
	while (*c) {
		if (cur_node->children[*c] == NULL)
			cur_node->children[*c] = trie_init();
		cur_node = cur_node->children[*c];
		c++;
	}
	cur_node->count = value;
}

size_t
trie_get(trie *t, const char *key)
{
	const char *c = key;
	trie *cur_node = t;
	while (*c) {
		if (cur_node->children[*c] == NULL)
			return 0;
		cur_node = cur_node->children[*c];
		c++;
	}
	return cur_node->count;
}
