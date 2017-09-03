/*-
 * Copyright (c) 2017 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
//#include <util.h>

#include "trie.h"

trie_t *
trie_init(void)
{
	return calloc(1, sizeof(trie_t));
}

void
trie_insert(trie_t **trie, const char *key, size_t value)
{
	char c = key[0];
	trie_t *t;
	if (*trie == NULL)
		*trie = trie_init();

	t = *trie;

	if (c == 0) {
		t->value = value;
		return;
	}

	if (t->character == 0)
		t->character = c;

	if (c > t->character)
		return trie_insert(&(t->right), key, value);

	if (c < t->character)
		return trie_insert(&(t->left), key, value);

	return trie_insert(&(t->middle), key + 1, value);
}

size_t
trie_get(trie_t * t, const char *key)
{
	char c = key[0];
	if (t == NULL)
		return 0;

	if (key[0] == 0)
		return t->value;

	if (t->character == 0)
		return 0;

	if (c == t->character)
		return trie_get(t->middle, key + 1);

	if (c > t->character)
		return trie_get(t->right, key);

	return trie_get(t->left, key);
}

void
trie_destroy(trie_t *t)
{
    if (t->left)
        trie_destroy(t->left);
    if (t->middle)
        trie_destroy(t->middle);
    if (t->right)
        trie_destroy(t->right);
    free(t);
}
