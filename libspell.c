/*-
 * Copyright (c) 2016 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
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

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "websters.c"
#include "libspell.h"


/*
 * This implementation is Based on the edit distance or Levenshtein distance technique.
 * Explained by Peter Norvig in his post here: http://norvig.com/spell-correct.html
 */

/*
 * Converts a word to lower case
 */
char *
lower(char *str)
{
	int i = 0;
	char c;
	while (str[i] != '\0') {
		c = tolower((unsigned char) str[i]);
		str[i++] = c;
	}
	return str;
}


/*
 * edits1--
 *  edits1 generates all permutations of the characters of a
 *  given word at maximum edit distance of 1.
 *
 *  All details are in the article mentioned at the top. But basically it generates 4
 *  types of possible arrangements of the chracters of a given word, stores them in an array and
 *  at the end returns that array to the caller. The 4 different arrangements
 *  are: (n = strlen(word) in the following description)
 *  1. Deletes: Delete one character at a time: n possible words
 *  2. Trasnposes: Change positions of two adjacent characters: n -1 possible words
 *  3. Replaces: Replace each character by one of the 26 alphabetes in English:
 *      26 * n possible words
 *  4. Inserts: Insert an alphabet at each of the character positions (one at a
 *      time. 26 * (n + 1) possible words.
 */
static char **
edits1(char *word)
{
	size_t i, len_a, len_b;
	char alphabet;
	size_t wordlen = strlen(word);
	size_t counter = 0;
	set splits[wordlen + 1];

	/* calculate number of possible permutations and allocate memory */
	size_t size = COMBINATIONS(wordlen);
	char **candidates = emalloc((size + 1) * sizeof(char *));

	/* Start by generating a split up of the characters in the word */
	for (i = 0; i < wordlen + 1; i++) {
		splits[i].a = (char *) emalloc(i + 1);
		splits[i].b = (char *) emalloc(wordlen - i + 1);
		memcpy(splits[i].a, word, i);
		memcpy(splits[i].b, word + i, wordlen - i + 1);
		splits[i].a[i] = 0;
	}

	/* Now generate all the permutations at maximum edit distance of 1.
	 * counter keeps track of the current index position in the array
	 * candidates where the next permutation needs to be stored. */
	for (i = 0; i < wordlen + 1; i++) {
		len_a = strlen(splits[i].a);
		len_b = strlen(splits[i].b);
		assert(len_a + len_b == wordlen);

		/* Deletes */
		if (i < wordlen) {
			char *candidate = emalloc(wordlen);
			memcpy(candidate, splits[i].a, len_a);
			if (len_b - 1 > 0)
				memcpy(candidate + len_a, splits[i].b + 1, len_b - 1);
			candidate[wordlen - 1] = 0;
			candidates[counter++] = candidate;
		}
		/* Transposes */
		if (i < wordlen - 1 && len_b >= 2 && splits[i].b[0] != splits[i].b[1]) {
			char *candidate = emalloc(wordlen + 1);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = splits[i].b[1];
			candidate[len_a + 1] = splits[i].b[0];
			memcpy(candidate + len_a + 2, splits[i].b + 2, len_b - 2);
			candidate[wordlen] = 0;
			candidates[counter++] = candidate;
		}
		/* For replaces and inserts, run a loop from 'a' to 'z' */
		for (alphabet = 'a'; alphabet <= 'z'; alphabet++) {
			/* Replaces */
			if (i < wordlen && splits[i].b[0] != alphabet) {
				char *candidate = emalloc(wordlen + 1);
				memcpy(candidate, splits[i].a, len_a);
				candidate[len_a] = alphabet;
				if (len_b - 1 >= 1)
					memcpy(candidate + len_a + 1, splits[i].b + 1, len_b - 1);
				candidate[wordlen] = 0;
				candidates[counter++] = candidate;
			}
			/* Inserts */
			char *candidate = emalloc(wordlen + 2);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = alphabet;
			if (len_b >= 1)
				memcpy(candidate + len_a + 1, splits[i].b, len_b);
			candidate[wordlen + 1] = 0;
			candidates[counter++] = candidate;
		}
	}
	candidates[counter] = NULL;

	for (i = 0; i < wordlen + 1; i++) {
		free(splits[i].a);
		free(splits[i].b);
	}
	return candidates;
}
/*
 * Takes a NULL terminated array of strings as input and returns a new array which
 * contains only those words which exist in the dictionary.
 */
static char **
get_corrections(char **candidate_list)
{
	if (candidate_list == NULL)
		return NULL;
	size_t i, corrections_count = 0;
	size_t corrections_size = 16;
	char **corrections = emalloc(corrections_size * sizeof(char *));

	while (candidate_list[i] != NULL) {
		char *candidate = candidate_list[i++];
		if (is_known_word(candidate))
			corrections[corrections_count++] = strdup(candidate);
		if (corrections_count == corrections_size - 1) {
			corrections_size *= 2;
			corrections = erealloc(corrections, corrections_size * sizeof(char *));
		}
	}
	corrections[corrections_count] = NULL;
	return corrections;
}

static int
max_count(const void *node1, const void *node2)
{
	word_count *wc1 = (word_count *) node1;
	word_count *wc2 = (word_count *) node2;
	if (wc1->count == wc2->count)
		return 0;

	return wc1->count > wc2->count? -1: 1;
}

static char **
spell_get_corrections(spell_t *spell, char **candidate_list, size_t n, int ngram)
{
	if (candidate_list == NULL)
		return NULL;
	size_t i = 0, corrections_count = 0;
	size_t corrections_size = 16;
	word_count *wc_array = emalloc(corrections_size * sizeof(*wc_array));

	while (candidate_list[i]) {
		char *candidate = candidate_list[i++];
		word_count node;
		word_count *tree_node;
		node.word = candidate;
		if (ngram == 1)
			tree_node = rb_tree_find_node(spell->dictionary, &node);
		else if (ngram == 2)
			tree_node = rb_tree_find_node(spell->ngrams_tree, &node);
		if (tree_node)
			wc_array[corrections_count++] = *tree_node;
		else
			continue;
		if (corrections_count == corrections_size - 1) {
			corrections_size *= 2;
			wc_array = erealloc(wc_array, corrections_size * sizeof(*wc_array));
		}
	}

	if (corrections_count == 0) {
		free(wc_array);
		return NULL;
	}

	char **corrections = emalloc((n + 1) * sizeof(char *));
	corrections[corrections_count] = NULL;
	qsort(wc_array, corrections_count, sizeof(*wc_array), max_count);
	for (i = 0; i < n; i++) {
		if (wc_array[i].word)
			corrections[i] =  estrdup(wc_array[i].word);
	}
	//XXX: Handle the case when n < corrections_count
	if (n < corrections_count)
		corrections[n] = NULL;
	return corrections;
}

void
free_list(char **list)
{
	size_t i = 0;
	if (list == NULL)
		return;

	while (list[i] != NULL)
		free(list[i++]);
	free(list);
}
/*
 * spell--
 *  The API exposed to the user. Returns the most closely matched word from the
 *  dictionary. It will first search for all possible words at distance 1, if no
 *  matches are found, it goes further and tries to look for words at edit
 *  distance 2 as well. If no matches are found at all, it returns NULL.
 */
char **
spell(char *word)
{
	char **corrections = NULL;
	char **candidates;
	lower(word);
	candidates = edits1(word);
	corrections = get_corrections(candidates);
	free_list(candidates);
	return corrections;
}
/*
 * Returns 1 if the word exists in the dictionary, 0 otherwise.
 * Callers should use this to decide whether they need to do
 * spell check for the word or not.
 */
int
is_known_word(const char *word)
{
	size_t len = strlen(word);
	unsigned int idx = dict_hash(word, len);
	return memcmp(dict[idx], word, len) == 0 && dict[idx][len] == '\0';
}


spell_t *
spell_init(const char *dictionary_path)
{
	FILE *f = fopen(dictionary_path, "r");
	if (f == NULL)
		return NULL;

	spell_t *spellt;
	static rb_tree_t *words_tree;
	static rb_tree_t *ngrams_tree;
	static const rb_tree_ops_t tree_ops = {
		.rbto_compare_nodes =  compare_words,
		.rbto_compare_key = compare_words,
		.rbto_node_offset = offsetof(word_count, rbtree),
		.rbto_context = NULL
	};

	words_tree = emalloc(sizeof(*words_tree));
	ngrams_tree = emalloc(sizeof(*ngrams_tree));
	rb_tree_init(words_tree, &tree_ops);
	rb_tree_init(ngrams_tree, &tree_ops);
	spellt = emalloc(sizeof(*spellt));
	spellt->dictionary = words_tree;
	spellt->ngrams_tree = ngrams_tree;

	char *word = NULL;
	char *line = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;
	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		char *tabindex = strchr(templine, '\t');
		if (tabindex == NULL) {
			free(words_tree);
			free(spellt);
			fclose(f);
			return NULL;
		}
		tabindex[0] = 0;
		word = estrdup(templine);
		templine = tabindex + 1;
		wcnode = emalloc(sizeof(*wcnode));
		wcnode->word = estrdup(word);
		wcnode->count = strtol(templine, NULL, 10);
		rb_tree_insert_node(words_tree, wcnode);
	}
	fclose(f);
	if ((f = fopen("dict/bigram.txt", "r")) == NULL)
		return spellt;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		char *tabindex = strchr(templine, '\t');
		if (tabindex == NULL) {
			free(words_tree);
			free(spellt);
			fclose(f);
			return spellt;
		}
		tabindex[0] = 0;
		word = estrdup(templine);
		templine = tabindex + 1;
		wcnode = emalloc(sizeof(*wcnode));
		wcnode->word = estrdup(word);
		wcnode->count = strtol(templine, NULL, 10);
		rb_tree_insert_node(ngrams_tree, wcnode);
	}
	fclose(f);
	return spellt;
}

int
spell_is_known_word(spell_t *spell, const char *word, int ngram)
{
	word_count wc;
	wc.word = (char *) word;
	word_count *node;
	if (ngram == 1)
		node = rb_tree_find_node(spell->dictionary, &wc);
	else if (ngram == 2)
		node = rb_tree_find_node(spell->ngrams_tree, &wc);
	return node != NULL? node->count: 0;
}

char **
spell_get_suggestions(spell_t *spell, char *word, int ngram)
{
	char **corrections = NULL;
	char **candidates;
	lower(word);
	candidates = edits1(word);
	corrections = spell_get_corrections(spell, candidates, 1, ngram);
	free_list(candidates);
	return corrections;
}

int
compare_words(void *context, const void *node1, const void *node2)
{
	const word_count *wc1 = (const word_count *) node1;
	const word_count *wc2 = (const word_count *) node2;

	return strcmp(wc1->word, wc2->word);
}
