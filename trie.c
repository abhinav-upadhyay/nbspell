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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

	if (t->character == 0)
		t->character = c;

	if (c > t->character)
		return trie_insert(&(t->right), key, value);

	if (c < t->character)
		return trie_insert(&(t->left), key, value);

	if (key[1]  == 0) {
		t->value = value;
		return;
	} else
		return trie_insert(&(t->middle), key + 1, value);
}

size_t
trie_get(trie_t * t, const char *key)
{
	char c = key[0];
	if (t == NULL)
		return 0;

	//if (key[0] == 0)
	//	return t->value;

	if (t->character == 0)
		return 0;

	if (c == t->character) {
		if (key + 1 == 0)
			return t->value;
		else
			return trie_get(t->middle, key + 1);
	}

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

trie_t *
get_subtrie(trie_t *t, const char *key)
{
	char c = key[0];
	if (t == NULL)
		return NULL;

	if (key[0] == 0)
		return NULL;

	if (t->character == 0)
		return NULL;

	if (c == t->character) {
		if (key[1] == 0)
			return t;
		else
			return get_subtrie(t->middle, key + 1);
	}

	if (c > t->character)
		return get_subtrie(t->right, key);

	return get_subtrie(t->left, key);
}

static void
collect(trie_t *t, const char *prefix, char **list, size_t *list_offset, size_t *list_size)
{
	if (t == NULL)
		return;

	collect(t->left, prefix, list, list_offset, list_size);
	char *new_prefix = NULL;
	asprintf(&new_prefix, "%s%c", prefix, t->character);
	if (t->value != 0) {
		if ((*list_offset) == (*list_size)) {
			size_t newsize = (*list_size) * 2;
			list = realloc(list, sizeof(*list) * newsize);
			*list_size = newsize;
		}
		list[*list_offset] = new_prefix;
		(*list_offset)++;
	}

	collect(t->middle, new_prefix, list, list_offset, list_size);
	collect(t->right, prefix, list, list_offset, list_size);
}

char **
get_prefix_matches(trie_t *t, const char *prefix)
{
	if (t == NULL || prefix == NULL)
		return NULL;

	trie_t *subtrie = get_subtrie(t, prefix);
	if (subtrie == NULL)
		return NULL;

	size_t list_size = 32;
	char **list = malloc(list_size * sizeof(*list));
	size_t list_offset = 0;

	if (subtrie->value != 0)
		list[list_offset++] = strdup(prefix);

	collect(subtrie->middle, prefix, list, &list_offset, &list_size);
	list[list_offset] = NULL;
	return list;
}
