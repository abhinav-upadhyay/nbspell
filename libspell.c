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

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

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
		*candidates = emalloc(sizeof(**candidates));
		(*candidates)->next = NULL;
		(*candidates)->word = candidate;
		(*candidates)->weight = weight;
		*tail = *candidates;
	} else {
		word_list *node = emalloc(sizeof(*node));
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
	char *word_soundex = soundex(word);
	char *candidate_soundex;
	const char alphabets[] = "abcdefghijklmnopqrstuvwxyz- ";

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
			float weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			weight /= 10;
			candidate_soundex = soundex(candidate);
			if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
				weight *= 20;
			free(candidate_soundex);
			add_candidate_node(candidate, &candidates, &tail, weight);
		}
		/* Transposes */
		if (i < wordlen - 1 && len_b >= 2 && splits[i].b[0] != splits[i].b[1]) {
			char *candidate = emalloc(wordlen + 1);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = splits[i].b[1];
			candidate[len_a + 1] = splits[i].b[0];
			memcpy(candidate + len_a + 2, splits[i].b + 2, len_b - 2);
			candidate[wordlen] = 0;
			float weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			candidate_soundex = soundex(candidate);
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
				char *candidate = emalloc(wordlen + 1);
				memcpy(candidate, splits[i].a, len_a);
				candidate[len_a] = alphabet;
				if (len_b - 1 >= 1)
					memcpy(candidate + len_a + 1, splits[i].b + 1, len_b - 1);
				candidate[wordlen] = 0;
				weight = 1.0 / distance;
				if (i == 0)
					weight /= 1000;
				candidate_soundex = soundex(candidate);
				if (word_soundex && candidate_soundex && strcmp(candidate_soundex, word_soundex) == 0)
					weight *= 20;
				weight /= 10;
				free(candidate_soundex);
				add_candidate_node(candidate, &candidates, &tail, weight);
			}
			/* Inserts */
			char *candidate = emalloc(wordlen + 2);
			memcpy(candidate, splits[i].a, len_a);
			candidate[len_a] = alphabet;
			if (len_b >= 1)
				memcpy(candidate + len_a + 1, splits[i].b, len_b);
			candidate[wordlen + 1] = 0;
			weight = 1.0 / distance;
			if (i == 0)
				weight /= 1000;
			weight *= 10;
			candidate_soundex = soundex(candidate);
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
	word_list *wl_array = emalloc(corrections_size * sizeof(*wl_array));
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
			wl_array = erealloc(wl_array, corrections_size * sizeof(*wl_array));
		}
	}

	if (corrections_count == 0) {
		free(wl_array);
		return NULL;
	}

	size_t arraysize = n < corrections_count? n: corrections_count;
	char **corrections = emalloc((arraysize + 1) * sizeof(*corrections));
	corrections[arraysize] = NULL;
	qsort(wl_array, corrections_count, sizeof(*wl_array), max_count);
	for (i = 0; i < arraysize; i++) {
		if (wl_array[i].word) {
			corrections[i] =  estrdup(wl_array[i].word);
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

		word = estrdup(templine);
		lower(word);
		wcnode = emalloc(sizeof(*wcnode));
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
	spellt = emalloc(sizeof(*spellt));
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

	if ((f = fopen("dict/bigram.txt", "r")) != NULL) {
		ngrams_tree = emalloc(sizeof(*ngrams_tree));
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
		soundex_tree = emalloc(sizeof(*soundex_tree));
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
				word_list *newlistnode = emalloc(sizeof(*newlistnode));
				newlistnode->word = estrdup(word);
				newlistnode->next = listnode->next;
				newlistnode->weight = .01;
				listnode->next = newlistnode;
			} else {
				listnode = emalloc(sizeof(*listnode));
				listnode->word = estrdup(soundex_code);
				word_list *newlistnode = emalloc(sizeof(*newlistnode));
				newlistnode->word = estrdup(word);
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
	char *soundex_code = emalloc(5);
	char *snd_buffer = estrdup(word);
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
		newnode = emalloc(sizeof(*newnode));
		newnode->word = estrdup(tempnode->word);
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
	char *soundex_code = soundex(word);
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
			corrections = emalloc(2 * sizeof(char *));
			corrections[0] = NULL;
			corrections[1] = NULL;
			int min_distance = 10000;
			word_list *node = soundexes->next;
			while (node != NULL) {
				if (edit_distance(word, node->word) < min_distance) {
					min_distance = edit_distance(word, node->word);
					if (corrections[0])
						free(corrections[0]);
					corrections[0] = estrdup(node->word);
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
		while((list = (word_list *) RB_TREE_MIN(spell->soundex_tree)) != NULL) {
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
