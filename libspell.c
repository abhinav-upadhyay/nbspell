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

typedef struct word_list {
	struct word_list *next;
	char *word;
	float weight;
	rb_node_t rbtree;
} word_list;

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
 */
static word_list *
edits1(char *word, size_t distance)
{
	size_t i, len_a, len_b;
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
			if (word_soundex && strcmp(candidate_soundex, word_soundex) == 0)
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
			if (word_soundex && strcmp(candidate_soundex, word_soundex) == 0)
				weight *= 20;
			free(candidate_soundex);
			add_candidate_node(candidate, &candidates, &tail, weight);
		}
		/* For replaces and inserts, run a loop from 'a' to 'z' */
		for (alphabet = 'a'; alphabet <= 'z'; alphabet++) {
			float weight = 1.0 / distance;
			/* Replaces */
			if (i < wordlen && splits[i].b[0] != alphabet) {
				char *candidate = emalloc(wordlen + 1);
				memcpy(candidate, splits[i].a, len_a);
				candidate[len_a] = alphabet;
				if (len_b - 1 >= 1)
					memcpy(candidate + len_a + 1, splits[i].b + 1, len_b - 1);
				candidate[wordlen] = 0;
				weight = 1.0 /distance;
				if (i == 0)
					weight /= 1000;
				candidate_soundex = soundex(candidate);
				if (word_soundex && strcmp(candidate_soundex, word_soundex) == 0)
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
			weight = 1.0 /distance;
			if (i == 0)
				weight /= 1000;
			weight *= 10;
			candidate_soundex = soundex(candidate);
			if (word_soundex && strcmp(candidate_soundex, word_soundex) == 0)
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

static word_list *
edits2(word_list *edits1_list)
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


/*
 * Takes a NULL terminated array of strings as input and returns a new array which
 * contains only those words which exist in the dictionary.
 */
static char **
get_corrections(word_list *candidate_list)
{
	if (candidate_list == NULL)
		return NULL;
	size_t i = 0;
	size_t corrections_count = 0;
	size_t corrections_size = 16;
	char **corrections = emalloc(corrections_size * sizeof(char *));
	word_list *nodep = candidate_list;

	while (nodep->next != NULL) {
		char *candidate = nodep->word;
		if (is_known_word(candidate))
			corrections[corrections_count++] = estrdup(candidate);
		if (corrections_count == corrections_size - 1) {
			corrections_size *= 2;
			corrections = erealloc(corrections, corrections_size * sizeof(char *));
		}
		nodep = nodep->next;
	}
	corrections[corrections_count] = NULL;
	return corrections;
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
spell_get_corrections(spell_t *spell, word_list *candidate_list, size_t n, int ngram)
{
	if (candidate_list == NULL)
		return NULL;

	size_t i = 0, corrections_count = 0;
	size_t corrections_size = 16;
	word_list *wc_array = emalloc(corrections_size * sizeof(*wc_array));
	word_list *nodep = candidate_list;
	float weight;

	while (nodep->next != NULL) {
		char *candidate = nodep->word;
		weight = nodep->weight;
		nodep = nodep->next;
		word_list listnode;
		word_count node;
		word_count *tree_node;
		node.word = candidate;
		if (ngram == 1)
			tree_node = rb_tree_find_node(spell->dictionary, &node);
		else if (ngram == 2)
			tree_node = rb_tree_find_node(spell->ngrams_tree, &node);
		if (tree_node) {
			listnode.weight = tree_node->count * weight;
			listnode.word = candidate;
			wc_array[corrections_count++] = listnode;
		} else
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

	size_t arraysize = n < corrections_count? n: corrections_count;
	char **corrections = emalloc((arraysize + 1) * sizeof(*corrections));
	corrections[arraysize] = NULL;
	qsort(wc_array, corrections_count, sizeof(*wc_array), max_count);
	for (i = 0; i < arraysize; i++) {
		if (wc_array[i].word) {
			corrections[i] =  estrdup(wc_array[i].word);
			//printf("%s %f\n", wc_array[i].word, wc_array[i].weight);
		}
	}
	free(wc_array);
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
	word_list *candidates;
	word_list *candidates2;
	lower(word);
	candidates = edits1(word, 1);
	corrections = get_corrections(candidates);
	if (corrections == NULL) {
		candidates2 = edits2(candidates);
		corrections = get_corrections(candidates);
		free_word_list(candidates2);
	}
	free_word_list(candidates);
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

static int
compare_listnodes(void *context, const void *node1, const void *node2)
{
	const word_list *wl1 = (const word_list *) node1;
	const word_list *wl2 = (const word_list *) node2;

	return strcmp(wl1->word, wl2->word);
}

static int
parse_file_and_generate_tree(FILE *f, rb_tree_t *tree)
{
	if (f == NULL)
		return -1;

	char *word = NULL;
	char *line = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		char *tabindex = strchr(templine, '\t');
		if (tabindex == NULL) {
			free(line);
			return -1;
		}
		tabindex[0] = 0;
		word = estrdup(templine);
		lower(word);
		templine = tabindex + 1;
		wcnode = emalloc(sizeof(*wcnode));
		wcnode->word = word;
		wcnode->count = strtol(templine, NULL, 10);
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
	FILE *f = fopen(dictionary_path, "r");
	if (f == NULL)
		return NULL;

	const rb_tree_ops_t tree_ops = {
		.rbto_compare_nodes =  compare_words,
		.rbto_compare_key = compare_words,
		.rbto_node_offset = offsetof(word_count, rbtree),
		.rbto_context = NULL
	};

	spell_t *spellt;
	rb_tree_t *words_tree;
	rb_tree_t *ngrams_tree;
	rb_tree_t *soundex_tree;

	words_tree = emalloc(sizeof(*words_tree));
	rb_tree_init(words_tree, &tree_ops);
	spellt = emalloc(sizeof(*spellt));
	spellt->dictionary = words_tree;
	spellt->ngrams_tree = NULL;
	spellt->whitelist = NULL;
	spellt->soundex_tree = NULL;

	char *word = NULL;
	char *line = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	if ((parse_file_and_generate_tree(f, words_tree)) < 0) {
		spell_destroy(spellt);
		fclose(f);
		return NULL;
	}
	fclose(f);

	if ((f = fopen("dict/bigram.txt", "r")) != NULL) {
		ngrams_tree = emalloc(sizeof(*ngrams_tree));
		rb_tree_init(ngrams_tree, &tree_ops);
		spellt->ngrams_tree = ngrams_tree;
		if ((parse_file_and_generate_tree(f, ngrams_tree)) < 0) {
			spell_destroy(spellt);
			fclose(f);
			return NULL;
		}
		fclose(f);
	}

	if ((f = fopen("dict/soundex.txt", "r")) != NULL) {
		const rb_tree_ops_t soundex_tree_ops = {
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
			if (line[bytes_read - 1] == '\r')
				line[bytes_read - 1] = 0;
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


	if (whitelist_filepath == NULL || (f = fopen(whitelist_filepath, "r")) == NULL)
		return spellt;

	rb_tree_t *whitelist;
	whitelist = emalloc(sizeof(*whitelist));
	rb_tree_init(whitelist, &tree_ops);
	spellt->whitelist = whitelist;

	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		word = estrdup(line);
		wcnode = emalloc(sizeof(*wcnode));
		wcnode->word = word;
		wcnode->count = 0;
		rb_tree_insert_node(whitelist, wcnode);
		free(line);
		line = NULL;
	}
	free(line);
	fclose(f);
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
		default:
			free(soundex_code);
			free(snd_buffer);
			return NULL;
		}
	}
	i = 1;
	c = 0;
	while (snd_buffer[i] != 0) {
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
		if (snd_buffer[i] == '-') {
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
spell_get_suggestions(spell_t * spell, char *word, int ngram)
{
	char **corrections = NULL;
	word_list *candidates;
	word_list *candidates2;
	word_list *soundexes;
	word_list soundex_node;
	word_list *tail;
	lower(word);
	candidates = edits1(word, 1);
	char *soundex_code;
	corrections = spell_get_corrections(spell, candidates, 1, ngram);
	if (corrections == NULL) {
		candidates2 = edits2(candidates);
		corrections = spell_get_corrections(spell, candidates2, 1, ngram);
		free_word_list(candidates2);
	}
	free_word_list(candidates);
	if (corrections == NULL) {
		soundex_code = soundex(word);
		if (soundex_code != NULL) {
			soundex_node.word = soundex_code;
			soundexes = rb_tree_find_node(spell->soundex_tree, &soundex_node);
			if (soundexes != NULL) {
				candidates = soundexes->next;
				corrections = spell_get_corrections(spell, candidates, 1, ngram);
			}
		}
		free(soundex_code);
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

int
is_whitelisted_word(spell_t *spell, const char *word)
{
	if (spell->whitelist == NULL)
		return 0;

	word_count wc;
	wc.word = (char *) word;
	word_count *node = rb_tree_find_node(spell->whitelist, &wc);
	return node != NULL;
}

/*
 * This is just a test function to inspect the list of words
 * generated by the edis1 function to make sure it is working
 * correctly. It should/will go away when I don't need to debug
 * this.
 */
void
print_edits(char *word)
{
	word_list *edits = edits1(word, 1);
	word_list *node = edits;
	while (node) {
		printf("%s\n", node->word);
		node = node->next;
	}
	free_word_list(edits);
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
	free_tree(spell->dictionary);

	if (spell->ngrams_tree != NULL)
		free_tree(spell->ngrams_tree);

	if (spell->whitelist != NULL)
		free_tree(spell->whitelist);
	free(spell);

}
