#include <stdio.h>

#include "trie.h"

int
main(int argc, char **argv)
{
	const char *s1 = "hello";
	const char *s2 = "world";
	const char *s3 = "hell";
	const char *s4 = "woo";

	trie_t *t = trie_init();
	trie_insert(&t, s1, 1);
	trie_insert(&t, s2, 4);
	trie_insert(&t, s3, 10);
	trie_insert(&t, s4, 5);

	printf("%s: %zu\n", s1, trie_get(t, s1));
	printf("%s: %zu\n", s2, trie_get(t, s2));
	printf("%s: %zu\n", s3, trie_get(t, s3));
	printf("%s: %zu\n", s4, trie_get(t, s4));
	return 0;
}
