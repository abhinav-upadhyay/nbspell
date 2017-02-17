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
#include <util.h>

#include "trie.h"

#define char_index(c) c - 97

trie *
trie_init(void)
{
	trie *t = emalloc(sizeof(*t));
	memset(t, 0, sizeof(*t));
	t->children = ecalloc(R_SIZE, sizeof(*(t->children)));
	return t;
}

void
trie_insert(trie *t, const char *key, size_t value)
{
	const char *c = key;
	trie *cur_node = t;
	while (*c) {
		if (cur_node->children[char_index(*c)] == NULL)
			cur_node->children[char_index(*c)] = trie_init();
		cur_node = cur_node->children[char_index(*c)];
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
		if (cur_node->children[char_index(*c)] == NULL)
			return 0;
		cur_node = cur_node->children[char_index(*c)];
		c++;
	}
	return cur_node->count;
}
