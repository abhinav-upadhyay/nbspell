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
	rb_tree_t *soundex_tree;
} spell_t;

void free_list(char **);
spell_t *spell_init(const char *, const char *);
int spell_is_known_word(spell_t *, const char *, int);
char **spell_get_suggestions(spell_t *, char *, size_t);
char *soundex(const char *);
void spell_destroy(spell_t *);
int compare_words(void *, const void *, const void *);
char *lower(char *);
char * sanitize_string(char *);
#endif
