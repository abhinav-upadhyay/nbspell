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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "dict.c"
#include "libspell.h"


/*
 * lower --
 *  Converts the string str to lower case
 */
static char *
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
 *  edits1 generates all permutations of a given word at maximum edit distance 
 *  of 1. All details are in the above article but basically it generates 4 
 *  types of possible permutations in a given word, stores them in an array and 
 *  at the end returns that array to the caller. The 4 different permutations 
 *  are: (n = strlen(word) in the following description)
 *  1. Deletes: Delete one character at a time: n possible permutations
 *  2. Trasnposes: Change positions of two adjacent characters: n -1 permutations
 *  3. Replaces: Replace each character by one of the 26 alphabetes in English:
 *      26 * n possible permutations
 *  4. Inserts: Insert an alphabet at each of the character positions (one at a
 *      time. 26 * (n + 1) possible permutations.
 */
static char **
edits1 (char *word)
{
	unsigned int i;
	size_t len_a;
	size_t len_b;
	unsigned int counter = 0;
	char alphabet;
	size_t wordlen = strlen(word);
	set splits[wordlen + 1];
	
	/* calculate number of possible permutations and allocate memory */
	size_t size = COMBINATIONS(wordlen);
	char **candidates = emalloc ((size + 1)  * sizeof(char *));

	/* Start by generating a split up of the characters in the word */
	for (i = 0; i < wordlen + 1; i++) {
		splits[i].a = (char *) emalloc(i + 1);
		splits[i].b = (char *) emalloc(wordlen - i + 1);
		memcpy(splits[i].a, word, i);
		memcpy(splits[i].b, word + i, wordlen - i + 1);
		splits[i].a[i] = 0;
	}

	/* Now generate all the permutations at maximum edit distance of 1.
	 * counter keeps track of the current index position in the array candidates
	 * where the next permutation needs to be stored.
	 */
	for (i = 0; i < wordlen + 1; i++) {
		len_a = strlen(splits[i].a);
		len_b = strlen(splits[i].b);
		assert(len_a + len_b == wordlen);

		/* Deletes */
		if (i < wordlen) {
			char *candidate = emalloc(wordlen);
			memcpy(candidate, splits[i].a, len_a);
			if (len_b -1 > 0)
				memcpy(candidate + len_a , splits[i].b + 1, len_b - 1);
			candidate[wordlen - 1] =0;
			candidates[counter++] = candidate;
		}

		/* Transposes */
		if (i < wordlen - 1) {
			char *candidate = emalloc(wordlen + 1);
			memcpy(candidate, splits[i].a, len_a);
			if (len_b >= 1)
				memcpy(candidate + len_a, splits[i].b + 1, 1);
			if (len_b >= 1)
				memcpy(candidate + len_a + 1, splits[i].b, 1);
			if (len_b >= 2)
				memcpy(candidate + len_a + 2, splits[i].b + 2, len_b - 2);
			candidate[wordlen] = 0;
			candidates[counter++] = candidate;
		}

		/* For replaces and inserts, run a loop from 'a' to 'z' */
		for (alphabet = 'a'; alphabet <= 'z'; alphabet++) {
			/* Replaces */
			if (i < wordlen) {
				char *candidate = emalloc(wordlen + 1);
				memcpy(candidate, splits[i].a, len_a);
				memcpy(candidate + len_a, &alphabet, 1);
				if (len_b - 1 >= 1)
					memcpy(candidate + len_a + 1, splits[i].b + 1, len_b - 1);
				candidate[wordlen] = 0;
				candidates[counter++] = candidate;
			}

			/* Inserts */
			char *candidate = emalloc(wordlen + 2);
			memcpy(candidate, splits[i].a, len_a);
			memcpy(candidate + len_a, &alphabet, 1);
			if (len_b >=1)
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
 *  Pass an array of strings to this function and it will return the word with 
 *  maximum frequency in the dictionary. If no word in the array list is found 
 *  in the dictionary, it returns NULL
 *  #TODO rename this function
 */
static char **
get_corrections(char **candidate_list)
{
	int i = 0;
	char **corrections = emalloc (16 * sizeof(char *));
	size_t corrections_count = 0;
	if (candidate_list == NULL)
		return NULL;

	while(candidate_list[i] != NULL) {
		char *candidate = candidate_list[i++];
		if (is_known_word(candidate))
			corrections[corrections_count++] = strdup(candidate);
	}
	corrections[corrections_count] = NULL;
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
	int i;
	char **corrections = NULL;
	char **candidates;
	char **cand2 = NULL;
	
	lower(word);
	candidates = edits1(word);
	corrections = get_corrections(candidates);
	/* No matches found ? Let's go further and find matches at edit distance 2.
	 * To make the search fast we use a heuristic. Take one word at a time from 
	 * candidates, generate it's permutations and look if a match is found.
	 * If a match is found, exit the loop. Works reasonably fast but accuracy 
	 * is not quite there in some cases.
	 */
	if (corrections == NULL) {
		for (i = 0; ; i++) {
			if (candidates[i] == NULL)
				break;
			cand2 = edits1(candidates[i]);
			if ((corrections = get_corrections(cand2)))
				break;
			else {
				free_list(cand2);
				cand2 = NULL;
			}
		}
	}
	free_list(candidates);
	free_list(cand2);
	return corrections;
}

/*
 * Returns 1 if the word exists in the dictionary, 0 otherwise.
 * Callers should use this to decide whether they need to do
 * spell check for the word or not.
 */
int
is_known_word(char *word)
{
	size_t len = strlen(word);
	unsigned int idx = dict_hash(word, len);
	return memcmp(dict[idx], word, len) == 0 && dict[idx][len] == '\0';
}
