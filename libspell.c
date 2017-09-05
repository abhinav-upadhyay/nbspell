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

#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include <bsd/util.h>

#include "libspell.h"
#include "trie.h"

typedef struct word_list {
	struct word_list *next;
	char *word;
	float weight;
	rb_node_t rbtree;
} word_list;

typedef struct set {
	char *a;
	char *b;
} set;

typedef struct next {
	char pri[2];
	char sec[2];
	size_t offset;
} next;


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

static void
free_word_list(word_list *list)
{
	if (list == NULL)
		return;

	word_list *nodep = list;
	word_list *temp;

	while (nodep != NULL) {
		free(nodep->word);
		temp = nodep->next;
		free(nodep);
		nodep = temp;
	}
}

static void
add_candidate_node(char *candidate, word_list **candidates, word_list **tail, float weight)
{
	if (*candidates == NULL) {
		*candidates = malloc(sizeof(**candidates));
		(*candidates)->next = NULL;
		(*candidates)->word = candidate;
		(*candidates)->weight = weight;
		*tail = *candidates;
	} else {
		word_list *node = malloc(sizeof(*node));
		node->word = candidate;
		node->next = NULL;
		node->weight = weight;
		(*tail)->next = node;
		*tail = node;
	}
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
 *
 *   This implementation is Based on the edit distance or Levenshtein distance technique.
 *   Explained by Peter Norvig in his post here: http://norvig.com/spell-correct.html
 */
static word_list *
edits1(char *word, size_t distance)
{
	size_t i, j, len_a, len_b;
	char alphabet;
	size_t wordlen = strlen(word);
	if (wordlen <= 1)
		return NULL;
	size_t counter = 0;
	set splits[wordlen + 1];
	word_list *candidates = NULL;
	word_list *tail = NULL;
	char *word_soundex = double_metaphone(word);
	char *candidate_soundex;
	const char alphabets[] = "abcdefghijklmnopqrstuvwxyz- ";

	/* Start by generating a split up of the characters in the word */
	for (i = 0; i < wordlen + 1; i++) {
		splits[i].a = (char *) malloc(i + 1);
		splits[i].b = (char *) malloc(wordlen - i + 1);
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
			char *candidate = malloc(wordlen);
			memcpy(candidate, splits[i].a, len_a);
			if (len_b - 1 > 0)
				memcpy(candidate + len_a, splits[i].b + 1, len_b - 1);
			candidate[wordlen - 1] = 0;
			float weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			weight /= 10;
			candidate_soundex = double_metaphone(candidate);
			if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
				weight *= 20;
			free(candidate_soundex);
			add_candidate_node(candidate, &candidates, &tail, weight);
		}
		/* Transposes */
		if (i < wordlen - 1 && len_b >= 2 && splits[i].b[0] != splits[i].b[1]) {
			char *candidate = malloc(wordlen + 1);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = splits[i].b[1];
			candidate[len_a + 1] = splits[i].b[0];
			memcpy(candidate + len_a + 2, splits[i].b + 2, len_b - 2);
			candidate[wordlen] = 0;
			float weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			candidate_soundex = double_metaphone(candidate);
			if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
				weight *= 20;
			free(candidate_soundex);
			add_candidate_node(candidate, &candidates, &tail, weight);
		}
		/* For replaces and inserts, run a loop from 'a' to 'z' */
		for (j = 0; j < sizeof(alphabets) - 1; j++) {
			alphabet = alphabets[j];
			float weight = 1.0 / distance;
			/* Replaces */
			if (i < wordlen && splits[i].b[0] != alphabet) {
				char *candidate = malloc(wordlen + 1);
				memcpy(candidate, splits[i].a, len_a);
				candidate[len_a] = alphabet;
				if (len_b - 1 >= 1)
					memcpy(candidate + len_a + 1, splits[i].b + 1, len_b - 1);
				candidate[wordlen] = 0;
				weight = 1.0 / distance;
				if (i == 0)
					weight /= 1000;
				candidate_soundex = double_metaphone(candidate);
				if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
					weight *= 20;
				weight /= 10;
				free(candidate_soundex);
				add_candidate_node(candidate, &candidates, &tail, weight);
			}
			/* Inserts */
			char *candidate = malloc(wordlen + 2);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = alphabet;
			if (len_b >= 1)
				memcpy(candidate + len_a + 1, splits[i].b, len_b);
			candidate[wordlen + 1] = 0;
			weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			weight *= 10;
			candidate_soundex = double_metaphone(candidate);
			if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
				weight *= 20;
			free(candidate_soundex);
			add_candidate_node(candidate, &candidates, &tail, weight);
		}
	}

	for (i = 0; i < wordlen + 1; i++) {
		free(splits[i].a);
		free(splits[i].b);
	}
	free(word_soundex);
	return candidates;
}

/*
 * Returns a list of words at an edit distance +1 than the words
 * in the list passed through the parameter edits1_list
 */
static word_list *
edits_plus_one(word_list *edits1_list)
{
	word_list *nodep = edits1_list;
	word_list *edits2_list = NULL;
	word_list *tail = edits1_list;
	word_list *templist;

	while (nodep->next != NULL) {
		templist = edits1(nodep->word, 2);
		nodep = nodep->next;
		if (templist == NULL)
			continue;

		if (edits2_list == NULL) {
			edits2_list = templist;
			tail = edits2_list;
		} else
			tail->next = templist;
		while (tail->next != NULL)
			tail = tail->next;
	}
	return edits2_list;
}


static int
max_count(const void *node1, const void *node2)
{
	word_list *wc1 = (word_list *) node1;
	word_list *wc2 = (word_list *) node2;
	if (wc1->weight == wc2->weight)
		return 0;

	return wc1->weight > wc2->weight ? -1: 1;
}

static char **
spell_get_corrections(spell_t *spell, word_list *candidate_list, size_t n)
{
	size_t i = 0, corrections_count = 0;
	size_t corrections_size = 16;
	word_list *wl_array = malloc(corrections_size * sizeof(*wl_array));
	word_list *nodep = candidate_list;
	float weight;

	if (candidate_list == NULL)
		return NULL;

	while (nodep->next != NULL) {
		char *candidate = nodep->word;
		weight = nodep->weight;
		nodep = nodep->next;
		word_list listnode;
		size_t count = trie_get(spell->dictionary, candidate);
		if (count == 0)
			continue;
		listnode.weight = count * weight;
		listnode.word = candidate;
		wl_array[corrections_count++] = listnode;

		if (corrections_count == corrections_size - 1) {
			corrections_size *= 2;
			wl_array = realloc(wl_array, corrections_size * sizeof(*wl_array));
		}
	}

	if (corrections_count == 0) {
		free(wl_array);
		return NULL;
	}

	size_t arraysize = n < corrections_count? n: corrections_count;
	char **corrections = malloc((arraysize + 1) * sizeof(*corrections));
	corrections[arraysize] = NULL;
	qsort(wl_array, corrections_count, sizeof(*wl_array), max_count);
	for (i = 0; i < arraysize; i++) {
		if (wl_array[i].word) {
			corrections[i] =  strdup(wl_array[i].word);
			//printf("%s %f\n", wl_array[i].word, wl_array[i].weight);
		}
	}
	free(wl_array);
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

static int
compare_listnodes(void *context, const void *node1, const void *node2)
{
	const word_list *wl1 = (const word_list *) node1;
	const word_list *wl2 = (const word_list *) node2;

	return strcmp(wl1->word, wl2->word);
}

static int
parse_file_and_generate_trie(FILE * f, trie_t * tree, char field_separator)
{
	if (f == NULL)
		return -1;

	char *line = NULL;
	size_t count;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		if (field_separator) {
			char *sepindex = strchr(templine, field_separator);
			if (sepindex == NULL) {
				free(line);
				return -1;
			}
			sepindex[0] = 0;
			count = strtol(sepindex + 1, NULL, 10);
		} else
			/* Since our trie expects to store a count of the
			 * frequency of the word and for some cases (such as
			 * the whitelist word file) we don't have those
			 * counts, set the default count as 1
			 */
			count = 1;

		lower(templine);
		trie_insert(&tree, templine, count);
		free(line);
		line = NULL;
	}
	free(line);
	return 0;
}

static int
parse_file_and_generate_tree(FILE *f, rb_tree_t *tree, char field_separator)
{
	if (f == NULL)
		return -1;

	char *word = NULL;
	char *line = NULL;
	long count;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		if (field_separator) {
			char *sepindex = strchr(templine, field_separator);
			if (sepindex == NULL) {
				free(line);
				return -1;
			}
			sepindex[0] = 0;
			count = strtol(sepindex + 1, NULL, 10);
		} else
			count = 1;

		word = strdup(templine);
		lower(word);
		wcnode = malloc(sizeof(*wcnode));
		wcnode->word = word;
		wcnode->count = count;
		rb_tree_insert_node(tree, wcnode);
		free(line);
		line = NULL;
	}
	free(line);
	return 0;
}

spell_t *
spell_init(const char *dictionary_path, const char *whitelist_filepath)
{
	FILE *f;

	static const rb_tree_ops_t tree_ops = {
		.rbto_compare_nodes =  compare_words,
		.rbto_compare_key = compare_words,
		.rbto_node_offset = offsetof(word_count, rbtree),
		.rbto_context = NULL
	};

	spell_t *spellt;
	trie_t *words_tree;
	static rb_tree_t *ngrams_tree;
	static rb_tree_t *soundex_tree;

	words_tree = trie_init();
	spellt = malloc(sizeof(*spellt));
	spellt->dictionary = words_tree;
	spellt->ngrams_tree = NULL;
	spellt->soundex_tree = NULL;

	char *word = NULL;
	char *line = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;

	if (whitelist_filepath != NULL && (f = fopen(whitelist_filepath, "r")) != NULL) {
		if ((parse_file_and_generate_trie(f, words_tree, 0)) < 0) {
			spell_destroy(spellt);
			fclose(f);
			return NULL;
		}
	}

	if ((f = fopen(dictionary_path, "r")) == NULL) {
		spell_destroy(spellt);
		return NULL;
	}

	if ((parse_file_and_generate_trie(f, words_tree, '\t')) < 0) {
		spell_destroy(spellt);
		fclose(f);
		return NULL;
	}
    fclose(f);

	if ((f = fopen("dict/bigram.txt", "r")) != NULL) {
		ngrams_tree = malloc(sizeof(*ngrams_tree));
		rb_tree_init(ngrams_tree, &tree_ops);
		spellt->ngrams_tree = ngrams_tree;
		if ((parse_file_and_generate_tree(f, ngrams_tree, '\t')) < 0) {
			spell_destroy(spellt);
			fclose(f);
			return NULL;
		}
		fclose(f);
	}

	if ((f = fopen("dict/soundex.txt", "r")) != NULL) {
		static const rb_tree_ops_t soundex_tree_ops = {
			.rbto_compare_nodes =  compare_listnodes,
			.rbto_compare_key = compare_listnodes,
			.rbto_node_offset = offsetof(word_list, rbtree),
			.rbto_context = NULL
		};
		soundex_tree = malloc(sizeof(*soundex_tree));
		rb_tree_init(soundex_tree, &soundex_tree_ops);
		spellt->soundex_tree = soundex_tree;
		while ((bytes_read = getline(&line, &linesize, f)) != -1) {
			line[--bytes_read] = 0;
			char *templine = line;
			char *tabindex = strchr(templine, '\t');
			if (tabindex == NULL) {
				free(soundex_tree);
				free(spellt);
				fclose(f);
				free(line);
				return spellt;
			}
			tabindex[0] = 0;
			char *soundex_code = templine;
			templine = tabindex + 1;
			word = templine;
			word_list *listnode;
			word_list templistnode;
			templistnode.word = soundex_code;
			listnode = rb_tree_find_node(soundex_tree, &templistnode);
			if (listnode != NULL) {
				word_list *newlistnode = malloc(sizeof(*newlistnode));
				newlistnode->word = strdup(word);
				newlistnode->next = listnode->next;
				newlistnode->weight = .01;
				listnode->next = newlistnode;
			} else {
				listnode = malloc(sizeof(*listnode));
				listnode->word = strdup(soundex_code);
				word_list *newlistnode = malloc(sizeof(*newlistnode));
				newlistnode->word = strdup(word);
				newlistnode->next = NULL;
				newlistnode->weight = .01;
				listnode->next= newlistnode;
				rb_tree_insert_node(soundex_tree, listnode);
			}
			free(line);
			line = NULL;
		}
		free(line);
		line = NULL;
		fclose(f);
	}
	free(line);
	return spellt;
}

char *
soundex(const char *word)
{
	if (word[0] == 0)
		return NULL;
	char *soundex_code = malloc(5);
	char *snd_buffer = strdup(word);
	lower(snd_buffer);
	snd_buffer[0] = toupper(snd_buffer[0]);
	int i = 1;
	char c;
	int soundex_len = 1;

	while (snd_buffer[i] != 0) {
		c = snd_buffer[i];
		switch (c) {
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
		case 'h':
		case 'y':
		case 'w':
			snd_buffer[i++] = '-';
			break;
		case 'b':
		case 'f':
		case 'p':
		case 'v':
			snd_buffer[i++] = '1';
			break;
		case 'c':
		case 'g':
		case 'j':
		case 'k':
		case 'q':
		case 's':
		case 'x':
		case 'z':
			snd_buffer[i++] = '2';
			break;
		case 'd':
		case 't':
			snd_buffer[i++] = '3';
			break;
		case 'l':
			snd_buffer[i++] = '4';
			break;
		case 'm':
		case 'n':
			snd_buffer[i++] = '5';
			break;
		case 'r':
			snd_buffer[i++] = '6';
			break;
		case '-':
		case '\'':
		case ' ':
		case '/':
			/* Ignore hyphens or apostrophies */
			snd_buffer[i++] = '_';
			break;
		default:
			free(soundex_code);
			free(snd_buffer);
			return NULL;
		}
	}
	i = 1;
	c = 0;
	while (snd_buffer[i] != 0) {
		if (snd_buffer[i] == '_') {
			i++;
			continue;
		}

		if (snd_buffer[i] == '-') {
			c = 0;
			i++;
			continue;
		}
		if (c == 0) {
			c = snd_buffer[i++];
			continue;
		}
		if (c == snd_buffer[i]) {
			snd_buffer[i++] = '-';
			continue;
		}
		c = snd_buffer[i++];
	}
	i = 1;
	soundex_code[0] = snd_buffer[0];
	while (snd_buffer[i] != 0 && soundex_len != 4) {
		if (snd_buffer[i] == '-' || snd_buffer[i] == '_') {
			i++;
			continue;
		}
		soundex_code[soundex_len++] = snd_buffer[i++];
	}
	while (soundex_len <= 4)
		soundex_code[soundex_len++] = '0';

	soundex_code[4] = 0;
	free(snd_buffer);
	return soundex_code;
}

static word_list *
copy_word_list(word_list * list)
{
	if (list == NULL)
		return NULL;

	word_list *newnode;
	word_list *tail = NULL;
	word_list *head = NULL;
	word_list *tempnode = list;
	while (tempnode != NULL) {
		newnode = malloc(sizeof(*newnode));
		newnode->word = strdup(tempnode->word);
		newnode->weight = tempnode->weight;
		newnode->next = NULL;
		if (head == NULL)
			head = newnode;
		if (tail == NULL)
			tail = newnode;
		else {
			tail->next = newnode;
			tail = newnode;
		}
		tempnode = tempnode->next;
	}
	return head;
}

static word_list *
get_soundex_list(spell_t *spell, char *word)
{
	word_list soundex_node;
	word_list *soundexes;
	char *soundex_code = double_metaphone(word);
	if (soundex_code == NULL)
		return NULL;

	soundex_node.word = soundex_code;
	soundexes = rb_tree_find_node(spell->soundex_tree, &soundex_node);
	free(soundex_code);
	if (soundexes == NULL)
		return NULL;

	return copy_word_list(soundexes->next);
}

static int
min(int i, int j, int k)
{
	int min = i;
	if (min > j)
		min = j;
	if (min > k)
		min = k;
	return min;
}

static int
edit_distance(const char *s1, const char *s2)
{
	size_t i, j;
	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);
	int m[len1 + 1][len2 + 1];
	for (i = 0; i < len1; i++)
		for(j = 0; j < len2; j++)
			m[i][j] = 0;

	for (i = 1; i <= len1; i++)
		m[i][0] = i;
	for (j = 1; j <= len2; j++)
		m[0][j] = j;

	for (i = 1; i <= len1; i++)
		for (j = 1; j <= len2; j++)
			if (s1[i - 1] == s2[j - 1])
				m[i][j] = m[i - 1][j - 1];
			else
				m[i][j] = min(m[i - 1][j - 1] + 1,
						m[i - 1][j] + 1,
						m[i][j - 1] + 1);
	return m[len1][len2];
}

int
spell_is_known_word(spell_t *spell, const char *word, int ngram)
{
	if (ngram == 1)
		return trie_get(spell->dictionary, word) != 0;
	else if (ngram == 2) {
		word_count wc;
		wc.word = (char *) word;
		word_count *node = rb_tree_find_node(spell->ngrams_tree, &wc);
		return node != NULL? node->count: 0;
	}
	return 0;
}

char **
spell_get_suggestions(spell_t * spell, char *word, size_t nsuggestions)
{
	char **corrections = NULL;
	word_list *candidates;
	word_list *candidates2;
	word_list *soundexes;
	word_list *tail;
	lower(word);
	candidates = edits1(word, 1);
	corrections = spell_get_corrections(spell, candidates, nsuggestions);
	if (corrections == NULL) {
		candidates2 = edits_plus_one(candidates);
		corrections = spell_get_corrections(spell, candidates2, nsuggestions);
		free_word_list(candidates2);
	}
	free_word_list(candidates);
	if (corrections == NULL) {
		soundexes = get_soundex_list(spell, word);
		if (soundexes != NULL) {
			corrections = malloc(2 * sizeof(char *));
			corrections[0] = NULL;
			corrections[1] = NULL;
			int min_distance = 10000;
			word_list *node = soundexes->next;
			while (node != NULL) {
				if (edit_distance(word, node->word) < min_distance) {
					min_distance = edit_distance(word, node->word);
					if (corrections[0])
						free(corrections[0]);
					corrections[0] = strdup(node->word);
				}
				node = node->next;
			}
			free_word_list(soundexes);
		}
	}
	return corrections;
}


int
compare_words(void *context, const void *node1, const void *node2)
{
	const word_count *wc1 = (const word_count *) node1;
	const word_count *wc2 = (const word_count *) node2;

	return strcmp(wc1->word, wc2->word);
}

static void
free_tree(rb_tree_t * tree)
{
	word_count *wc;
	while ((wc = RB_TREE_MIN(tree)) != NULL) {
		rb_tree_remove_node(tree, wc);
		free(wc->word);
		free(wc);
	}
	free(tree);
}

void
spell_destroy(spell_t * spell)
{
    word_list *list;
    trie_destroy(spell->dictionary);
	if (spell->ngrams_tree != NULL)
		free_tree(spell->ngrams_tree);
	if (spell->soundex_tree != NULL) {
		while ((list = (word_list *) RB_TREE_MIN(spell->soundex_tree)) != NULL) {
			rb_tree_remove_node(spell->soundex_tree, list);
			free_word_list(list);
		}
		free(spell->soundex_tree);
	}
	free(spell);
}

char *
sanitize_string(char *s)
{
	size_t len = strlen(s);
	int i = 0;
	if (s[0] == '(' && s[len - 1] == ')') {
		s[--len] = 0;
		s++;
		--len;
	}
	char *ret = malloc(len + 1);
	memset(ret, 0, len + 1);
	while (*s) {
		/*
		 * Detect apostrophe and stop copying characters immediately
		 */
		if ((*s == '\'') && (
			!strncmp(s + 1, "s", 1) ||
			!strncmp(s + 1, "es", 2) ||
			!strncmp(s + 1, "m", 1) ||
			!strncmp(s + 1, "d", 1) ||
			!strncmp(s + 1, "ll", 2))) {
			break;
		}
		/*
		 * If the word contains a dot in between that suggests it is either
		 * an abbreviation or somekind of a URL. Do not bother with such words.
		 */
		if (*s == '.') {
			free(ret);
			return NULL;
		}
		//Why bother with words which contain other characters or numerics ?
		if (!isalpha(*s)) {
			free(ret);
			return NULL;
		}
		ret[i++] = *s++;
	}
	ret[i] = 0;
	return ret;
}

static int
is_vowel(char c)
{
	if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'Y')
		return 1;
	return 0;
}

static char *
to_upper(const char *s)
{
	char *u = strdup(s);
	size_t i = 0;
	while(u[i] != 0) {
		u[i] = toupper((int) u[i]);
		i++;
	}
	return u;
}

static char *
pad(const char *s)
{
	char *ret = NULL;
	asprintf(&ret, "--%s------", s);
	if (ret == NULL)
		err(EXIT_FAILURE, "malloc failed");
	return ret;
}

static int
is_slavo_germanic(const char *s)
{
	char prev_ch = s[0];
	const char *d = s + 1;
	while (*d) {
		if (*d == 'W')
			return 1;
		if (*d == 'K')
			return 1;
		if (prev_ch == 'C' && *d == 'Z')
			return 1;
		prev_ch = *d;
		d++;
	}
	return 0;
}

static int
is_in(const char *s, size_t len, size_t count, ...)
{
	va_list ap;
	va_start(ap, count);
	size_t i;
	for (i = 0; i < count; i++)
		if (strncmp(s, va_arg(ap, char *), len) == 0)
			return 1;
	return 0;
}

char *
double_metaphone(const char *s)
{
	char *st = to_upper(s);
	int is_sl_germanic = is_slavo_germanic(s);
	size_t len = strlen(st);
	size_t first = 2;
	size_t last = first + len - 1;
	size_t pos = first;
	char *pri = malloc(len + 1);
	char *sec = malloc(len + 1);
	size_t pri_offset = 0;
	size_t sec_offset = 0;
	struct next nxt;
	char *padded = pad(st);
	free(st);
	st = padded;

	if ((st[first] == 'G' && st[first + 1] == 'N') ||
	    (st[first] == 'K' && st[first + 1] == 'N') ||
	    (st[first] == 'P' && st[first + 1] == 'N') ||
	    (st[first] == 'W' && st[first + 1] == 'R') ||
	    (st[first] == 'P' && st[first + 1] == 'S')
	    )
		pos++;

	if (st[first] == 'X') {
		pri[pri_offset++] = 'S';
		pos++;
	}
	while (pos <= last) {
		char ch = st[pos];
		nxt.pri[0] = 0;
		nxt.pri[1] = 0;
		nxt.sec[0] = 0;
		nxt.sec[1] = 0;
		nxt.offset = 1;
		if (is_vowel(ch)) {
			nxt.pri[0] = 0;
			nxt.pri[1] = 0;
			nxt.offset = 1;
			if (pos == first) {
				nxt.pri[0] = 'A';
				nxt.offset = 1;
			}
		} else if (ch == 'B') {
			if (st[pos + 1] == 'B') {
				nxt.pri[0] = 'P';
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'P';
				nxt.offset = 1;
			}
		} else if (ch == 'C') {
			if (pos > first + 1 && !is_vowel(st[pos - 2]) &&
			    strncmp(&st[pos - 1], "ACH", 3) &&
			    st[pos + 2] != 'I' &&
			    (st[pos + 2] != 'E' || is_in(&st[pos - 2], 6, 2, "BACHER", "MACHER"))) {
				nxt.pri[0] = 'K';
				nxt.offset = 2;
			} else if (pos == first && strncmp(&st[first], "CAESAR", 6) == 0) {
				nxt.pri[0] = 'S';
				nxt.offset = 2;
			} else if (strncmp(&st[pos], "CHIA", 4) == 0) {
				nxt.pri[0] = 'K';
				nxt.offset = 2;
			} else if (strncmp(&st[pos], "CH", 2) == 0) {
				if (pos > first && strncmp(&st[pos], "CHAE", 4) == 0) {
					nxt.pri[0] = 'K';
					nxt.sec[0] = 'X';
					nxt.offset = 2;
				} else if (pos == first && (
						is_in(&st[pos + 1], 5, 2, "HARAC", "HARIS") ||
						is_in(&st[pos + 1], 3, 4, "HOR", "HYM", "HIA", "HEM")
				    ) && strncmp(&st[first], "CHORE", 5) == 1) {
					nxt.pri[0] = 'K';
					nxt.offset = 2;
				} else if (is_in(&st[first], 4, 2, "VAN ", "VON ") ||
					    strncmp(&st[first], "SCH", 3) == 0 ||
					    is_in(&st[pos - 2], 6, 3, "ORCHES", "ARCHIT", "ORCHID") ||
					    (st[pos + 2] == 'T' ||
						st[pos + 2] == 'S') ||
					    ((is_in(&st[pos - 1], 1, 4, "A", "O", "U", "E") ||
						    pos == first) &&
					is_in(&st[pos + 2], 1, 9, "L", "R", "N", "M", "B", "H", "F", "V", "W"))) {
					nxt.pri[0] = 'A';
					nxt.offset = 2;
				} else {
					if (pos > first) {
						if (st[first] == 'M' && st[first + 1] == 'C') {
							nxt.pri[0] = 'K';
							nxt.offset = 2;
						} else {
							nxt.pri[0] = 'X';
							nxt.sec[0] = 'K';
							nxt.offset = 2;
						}
					} else {
						nxt.pri[0] = 'X';
						nxt.offset = 2;
					}
				}
			} else if (strncmp(&st[pos + 2], "CZ", 2) == 0 && strncmp(&st[pos - 2], "WICZ", 4) == 0) {
				nxt.pri[0] = 'S';
				nxt.sec[0] = 'X';
				nxt.offset = 2;
			} else if (strncmp(&st[pos + 1], "CIA", 3) == 0) {
				nxt.pri[0] = 'X';
				nxt.offset = 3;
			} else if (strncmp(&st[pos], "CC", 2) == 0 && !(pos == first + 1 && st[first] == 'M')) {
				if (is_in(&st[pos + 2], 1, 3, "I", "E", "H") && strncmp(&st[pos + 2], "HU", 2) == 1) {
					if ((pos == first + 1 && st[first] == 'A') || is_in(&st[pos - 1], 5, 2, "UCCEE", "UCCES")) {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 'S';
						nxt.offset = 3;
					} else {
						nxt.pri[0] = 'X';
						nxt.pri[1] = 0;
						nxt.offset = 3;
					}
				} else {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 2, 3, "CK", "CG", "CQ")) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (is_in(&st[pos], 2, 3, "CI", "CE", "CY")) {
				if (is_in(&st[pos], 3, 3, "CIO", "CIE", "CIA")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'X';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'S';
					nxt.offset = 2;
				}
			} else {
				if (is_in(&st[pos + 1], 2, 3, " C", " Q", " G")) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					if (is_in(&st[pos + 1], 1, 3, "C", "K", "Q") && !is_in(&st[pos + 1], 2, 2, "CE", "CI")) {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 0;
						nxt.offset = 1;
					}
				}
			}
		} else if (ch == 'D') {
			if (strncmp(&st[pos], "DG", 2) == 0) {
				if (st[pos + 2] == 'I' || st[pos + 2] == 'E' || st[pos + 2] == 'Y') {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'T';
					nxt.pri[1] = 'K';
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 2, 2, "DT", "DD")) {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'F') {
			if (st[pos + 1] == 'F') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'G') {
			if (st[pos + 1] == 'H') {
				if (pos > first && !is_vowel(st[pos - 1])) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else if (pos < first + 3) {
					if (pos == first) {
						if (st[pos + 2] == 'I') {
							nxt.pri[0] = 'J';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						} else {
							nxt.pri[0] = 'K';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						}
					}
				} else if ((pos > first + 1 && is_in(&st[pos - 2], 1, 3, "B", "H", "D")) ||
					    (pos > first + 2 && is_in(&st[pos - 3], 1, 3, "B", "H", "D")) ||
				    (pos > first + 3 && is_in(&st[pos - 3], 1, 2, "B", "H"))) {
					nxt.pri[0] = 0;
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					if (pos > first + 2 && st[pos - 1] == 'U' &&
					    is_in(&st[pos - 3], 1, 5, "C", "G", "L", "R", "T")) {
						nxt.pri[0] = 'F';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						if (pos > first && st[pos - 1] != 'I') {
							nxt.pri[0] = 'K';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						}
					}
				}
			} else if (st[pos + 1] == 'N') {
				if (pos == first + 1 && is_vowel(st[first]) && !is_sl_germanic) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 'N';
					nxt.sec[0] = 'N';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 'N';
					nxt.offset = 2;
				}
			} else if (strncmp(&st[pos + 1], "LI", 2) == 0 && !is_sl_germanic) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 'L';
				nxt.sec[0] = 'L';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if (pos == first && (st[pos + 1] == 'Y' ||
					is_in(&st[pos + 1], 2, 11, "ES", "EP", "EB", "EL",
				    "EY", "IB", "IL", "IN", "IE", "EI", "ER"))) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'J';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if ((strncmp(&st[pos + 1], "ER", 2) == 0 || st[pos + 1] == 'Y') &&
				    !is_in(&st[first], 6, 3, "DANGER", "RANGER", "MANGER") &&
				    !is_in(&st[pos - 1], 1, 2, "E", "I") &&
			    !is_in(&st[pos - 1], 3, 2, "RGY", "OGY")) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'J';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if (is_in(&st[pos + 1], 1, 3, "E", "I", "Y") ||
			    is_in(&st[pos - 1], 4, 2, "AGGI", "OGGI")) {
				if (is_in(&st[first], 4, 2, "VON ", "VAN ") ||
				    strncmp(&st[first], "SCH", 3) == 0 ||
				    strncmp(&st[pos + 1], "ET", 2) == 0) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					if (strncmp(&st[pos + 1], "IER ", 4) == 0) {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.sec[0] = 'K';
						nxt.sec[1] = 0;
						nxt.offset = 2;
					}
				}
			} else if (st[pos + 1] == 'G') {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'H') {
			if ((pos == first || is_vowel(st[pos - 1])) &&
			    is_vowel(st[pos + 1])) {
				nxt.pri[0] = 'H';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 2;
			}
		} else if (ch == 'J') {
			if (strncmp(&st[pos], "JOSE", 4) == 0 ||
			    strncmp(&st[first], "SAN ", 4) == 0) {
				if ((pos == first && st[pos + 4] == ' ') ||
				    strncmp(&st[first], "SAN ", 4) == 0) {
					nxt.pri[0] = 'H';
					nxt.pri[1] = 0;
				} else {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'H';
					nxt.sec[1] = 0;
				}
			} else if (pos == first && strncmp(&st[pos], "JOSE", 4) == 1) {
				nxt.pri[0] = 'J';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'A';
				nxt.sec[1] = 0;
			} else {
				if (is_vowel(st[pos - 1]) && !is_sl_germanic &&
				    is_in(&st[pos + 1], 1, 2, "A", "O")) {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'H';
					nxt.sec[1] = 0;
				} else {
					if (pos == last) {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.sec[0] = ' ';
						nxt.sec[1] = 0;
					} else {
						if (!is_in(&st[pos + 1], 1, 8, "L", "T", "K", "S", "N",
							"M", "B", "Z") &&
						    !is_in(&st[pos - 1], 1, 3, "S", "K", "L")) {
							nxt.pri[0] = 'J';
							nxt.pri[1] = 0;
						} else {
							nxt.pri[0] = 0;
							nxt.pri[1] = 0;
						}
					}
				}
			}
			if (st[pos + 1] == 'J') {
				nxt.offset = 2;
			} else {
				nxt.offset = 1;
			}
		} else if (ch == 'K') {
			if (st[pos + 1] == 'L') {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'L') {
			if (st[pos + 1] == 'L') {
				if ((pos == last - 2 &&
					is_in(&st[pos - 1], 4, 3, "ILLO", "ILLA", "ALLE")) ||
				    ((is_in(&st[last - 1], 2, 2, "AS", "OS") ||
					    st[last] == 'A' || st[last] == 'O') &&
					strncmp(&st[pos - 1], "ALLE", 4) == 0)) {
					nxt.pri[0] = 'L';
					nxt.pri[1] = 0;
					nxt.sec[0] = ' ';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'L';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else {
				nxt.pri[0] = 'L';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'M') {
			if ((strncmp(&st[pos + 1], "UMB", 3) == 0 &&
				(pos + 1 == last ||
				    strncmp(&st[pos + 2], "ER", 2) == 0)) ||
			    st[pos + 1] == 'M') {
				nxt.pri[0] = 'M';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'M';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'N') {
			if (st[pos + 1] == 'N') {
				nxt.pri[0] = 'N';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'N';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'P') {
			if (st[pos + 1] == 'H') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (st[pos + 1] == 'P' || st[pos + 1] == 'B') {
				nxt.pri[0] = 'P';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'P';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'Q') {
			if (st[pos + 1] == 'Q') {
				nxt.pri[0] = 'Q';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'Q';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'R') {
			if (pos == last && !is_sl_germanic &&
			    strncmp(&st[pos - 2], "IE", 2) == 0 &&
			    !is_in(&st[pos - 4], 2, 2, "ME", "MA")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.sec[0] = 'R';
				nxt.sec[1] = 0;
			} else {
				nxt.pri[0] = 'R';
				nxt.pri[1] = 0;
			}
			if (st[pos + 1] == 'R')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		} else if (ch == 'S') {
			if (is_in(&st[pos - 1], 3, 2, "ISL", "YSL")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			} else if (pos == first && strncmp(&st[first], "SUGAR", 5) == 0) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'S';
				nxt.sec[1] = 0;
				nxt.offset = 1;
			} else if (strncmp(&st[pos], "SH", 2) == 0) {
				if (is_in(&st[pos + 1], 4, 4, "HEIM", "HOEK", "HOLM", "HOLZ")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'X';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 3, 2, "SIO", "SIA") ||
			    strncmp(&st[pos], "SIAN", 4) == 0) {
				if (!is_sl_germanic) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'X';
					nxt.sec[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				}
			} else if ((pos == first &&
					is_in(&st[pos + 1], 1, 4, "M", "N", "L", "W")) ||
			    st[pos + 1] == 'Z') {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'X';
				nxt.sec[1] = 0;
				if (st[pos + 1] == 'Z')
					nxt.offset = 2;
				else
					nxt.offset = 1;
			} else if (strncmp(&st[pos], "SC", 2) == 0) {
				if (st[pos + 2] == 'H') {
					if (is_in(&st[pos + 3], 2, 6, "OO", "ER", "EN", "UY", "ED", "EM")) {
						if (is_in(&st[pos + 3], 2, 2, "ER", "EN")) {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.sec[0] = 'S';
							nxt.sec[1] = 'K';
							nxt.offset = 3;
						} else {
							nxt.pri[0] = 'S';
							nxt.pri[1] = 'K';
							nxt.offset = 3;
						}
					} else {
						if (pos == first && !is_vowel(st[first + 3]) &&
						    st[first + 3] != 'W') {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.sec[0] = 'S';
							nxt.sec[1] = 0;
							nxt.offset = 3;
						} else {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.offset = 3;
						}
					}
				} else if (is_in(&st[pos + 2], 1, 3, "I", "E", "Y")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 'K';
					nxt.offset = 3;
				}
			} else if (pos == last && is_in(&st[pos - 2], 2, 2, "AI", "OI")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			} else {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				if (st[pos + 1] == 'S' || st[pos + 1] == 'Z')
					nxt.offset = 2;
				else
					nxt.offset = 1;
			}
		} else if (ch == 'T') {
			if (strncmp(&st[pos], "TION", 4) == 0) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.offset = 3;
			} else if (is_in(&st[pos], 3, 2, "TIA", "TCH")) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.offset = 3;
			} else if (strncmp(&st[pos], "TH", 2) == 0 ||
			    strncmp(&st[pos], "TTH", 3) == 0) {
				if (is_in(&st[pos + 2], 2, 2, "OM", "AM") ||
				    is_in(&st[first], 4, 2, "VON ", "VAN ") ||
				    strncmp(&st[first], "SCH", 3) == 0) {
					nxt.pri[0] = 'T';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = '0';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'T';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				}
			} else if (st[pos + 1] == 'T' || st[pos + 1] == 'D') {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'V') {
			if (st[pos + 1] == 'V') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'W') {
			if (strncmp(&st[pos], "WR", 2) == 0) {
				nxt.pri[0] = 'R';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (pos == first && (is_vowel(st[pos + 1]) ||
				strncmp(&st[pos], "WH", 2) == 0)) {
				if (is_vowel(st[pos + 1])) {
					nxt.pri[0] = 'A';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'F';
					nxt.sec[1] = 0;
					nxt.offset = 1;
				} else {
					nxt.pri[0] = 'A';
					nxt.pri[1] = 0;
					nxt.offset = 1;
				}
			} else if ((pos == last && is_vowel(st[pos - 1])) ||
				    is_in(&st[pos - 1], 5, 4, "EWSKI", "EWSKY", "OWSKI", "OWSKY") ||
			    strncmp(&st[first], "SCH", 3) == 0) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.sec[0] = 'F';
				nxt.sec[1] = 0;
				nxt.offset = 1;
			} else if (is_in(&st[pos], 4, 2, "WICZ", "WITZ")) {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 'S';
				nxt.sec[0] = 'F';
				nxt.sec[1] = 'X';
				nxt.offset = 4;
			} else {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'X') {
			nxt.pri[0] = 0;
			nxt.pri[1] = 0;
			if (!(pos == last && (is_in(&st[pos - 3], 3, 2, "IAU", "EAU") ||
				    is_in(&st[pos - 2], 2, 2, "AU", "OU")))) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 'S';
			}
			if (st[pos + 1] == 'C' || st[pos + 1] == 'X')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		} else if (ch == 'Z') {
			if (st[pos + 1] == 'H') {
				nxt.pri[0] = 'J';
				nxt.pri[1] = 0;
			} else if (is_in(&st[pos + 1], 2, 3, "ZO", "ZI", "ZA") ||
			    (is_sl_germanic && pos > first && st[pos - 1] != 'T')) {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'T';
				nxt.sec[1] = 'S';
			} else {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
			}
			if (st[pos + 1] == 'Z' || st[pos + 1] == 'H')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		}
		if (nxt.sec[0] == 0) {
			if (nxt.pri[0]) {
				pri[pri_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					pri[pri_offset++] = nxt.pri[1];
				sec[sec_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					sec[sec_offset++] = nxt.pri[1];

			}
			pos += nxt.offset;
		} else {
			if (nxt.pri[0]) {
				pri[pri_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					pri[pri_offset++] = nxt.pri[1];
			}
			if (nxt.sec[0])
				sec[sec_offset++] = nxt.sec[0];
			if (nxt.sec[1])
				sec[sec_offset++] = nxt.sec[1];
			pos += nxt.offset;
		}
	}
    free(st);
	pri[pri_offset] = 0;
	sec[sec_offset] = 0;
    free(sec);
	return pri;
}

