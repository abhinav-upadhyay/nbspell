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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <util.h>

#include "libspell.h"


static void
usage(void)
{
    (void) fprintf(stderr, "Usage: bigspell [-i input_file] [-n ngram] [-w whitelist]\n");
    exit(1);
}

static char *
get_next_word(FILE *inputf, char **line, int *sentence_end)
{
    ssize_t bytes_read;
    size_t linesize = 0;
    size_t wordsize;
	char *word;
	char *sanitized_word;
    if (*line == NULL || (*line)[0] == 0) {
		*line = NULL;
        bytes_read = getline(line, &linesize, inputf);
        if (bytes_read == -1)
            return NULL;
        (*line)[--bytes_read] = 0;
        if ((*line)[bytes_read] == '\r' || (*line)[bytes_read] == '\n')
            (*line)[bytes_read] = 0;
    }

    char *templine = *line;
    wordsize = strcspn(templine, "()<>@?\'\",;-:. \t");
    switch (templine[wordsize]) {
        case '?':
        case '.':
        case ';':
        case '-':
        case '\t':
        case '(':
        case ')':
            *sentence_end = 1;
            break;
    }

    templine[wordsize] = 0;
    word = templine;
    templine += wordsize + 1;
    while (*templine == ' ')
        templine++;
    lower(word);
	sanitized_word = sanitize_string(word);
	if (!sanitized_word || !sanitized_word[0]) {
		free(sanitized_word);
		sanitized_word = NULL;
	}
    *line = templine;
	if (sanitized_word == NULL || strlen(sanitized_word) == 0)
		return get_next_word(inputf, line, sentence_end);
    return sanitized_word;
}

static void
print_word_list(word_list *suggestions, char *word)
{
	if (suggestions == NULL)
		return;
	word_list *node = suggestions;
	size_t i = 0;
	printf("%s: ", word);
	for (; node; node = node->next) {
		if (i > 0)
			printf("%s", ",");
		printf("%s", node->word);
		i++;
	}
	printf("\n");
}

static char *
get_best_match(spell_t *spellt, word_list *suggestions, char *word, int word_position)
{
	word_list *node = suggestions;
	char *best_match = NULL;
	size_t max = 0;
	char *bigram_word = NULL;
	size_t max_frequency = 0;

	for (; node; node = node->next) {
		if (word_position == 1)
			asprintf(&bigram_word, "%s %s", node->word, word);
		else
			asprintf(&bigram_word, "%s %s", word, node->word);
		int suggestion_frequency = spell_is_known_word(spellt, bigram_word, 2);
		if (suggestion_frequency > max_frequency) {
			max_frequency = suggestion_frequency;
			best_match = node->word;
		}
		free(bigram_word);
		bigram_word = NULL;
	}
	if (best_match) {
//		printf("%s: %s\n", word, best_match);
		return strdup(best_match);
	} else {
//		printf("%s: %s\n", word, suggestions->word);
		return strdup(suggestions->word);
	}
}

static void
do_bigram(FILE *inputf, const char *whitelist_filepath, size_t nsuggestions)
{

    char *curword = NULL;
    spell_t *spellt = spell_init("dict/unigram.txt", whitelist_filepath);
    load_bigrams(spellt, "dict/bigram.txt");
    char *line = NULL;
    word_count *wcnode;
    word_count wc;
    wc.count = 0;
    char *prevword = NULL;
    char *nextword = NULL;
    char *bigram_word = NULL;
    char *correction = NULL;
    size_t i;
    int sentence_end = 0;

    while ((curword = get_next_word(inputf, &line, &sentence_end)) != NULL) {
        if (spell_is_known_word(spellt, curword, 1)) {
            if (prevword != NULL)
                free(prevword);
			if (!sentence_end)
				prevword = curword;
			else {
				free(prevword);
				prevword = NULL;
			}
            continue;
        }

        /*
         * Beginning of a sentence, so we read the next word and use
		 * curword + nextword to form a bigram
         */
        if (prevword == NULL) {
            nextword = get_next_word(inputf, &line, &sentence_end);

			/*
			 * No more words left to process, just do a unigram check and return
			 */
            if (nextword == NULL) {
                word_list *wl  = spell_get_suggestions_slow(spellt, nextword, nsuggestions);
				print_word_list(wl, nextword);
				free_word_list(wl);
                free(curword);
				spell_destroy(spellt);
				free(line);
                return;
            }

			/*
			 * Next word is a known word, use it to guess the previous misspelled word
			 */
            if (spell_is_known_word(spellt, nextword, 1)) {
                word_list *suggestions = spell_get_suggestions_slow(spellt, curword, nsuggestions);
                char *best_match = get_best_match(spellt, suggestions, curword, 1);
				if (best_match)
					printf("%s: %s\n", curword, best_match);
                free_word_list(suggestions);
                free(curword);
				if (!sentence_end) 
					prevword = nextword;
				free(best_match);
                continue;
            }

			/*
			 * Next word is either an unknown or a misspelled word, do our best
			 * to guess the current and next words
			 */
            word_list *curword_suggestions = spell_get_suggestions_slow(spellt, curword, nsuggestions);
            word_list *nextword_suggestions = spell_get_suggestions_slow(spellt, nextword, nsuggestions);
            word_list *node;

			/* No suggestions for either:
			 * Since we don't know the nextword and can't find any corrections
			 * for it, set prevword = NULL and continue
			 */
            if (nextword_suggestions == NULL && curword_suggestions == NULL) {
                free(curword);
                prevword = NULL;
                continue;
            }

			/* No suggestions for the current word
			 * we will set the prevword as nextword
			 */
            if (curword_suggestions == NULL) {
				printf("%s: %s\n", nextword, nextword_suggestions->word);
				if (!sentence_end)
					prevword = strdup(nextword_suggestions->word);
                free_word_list(nextword_suggestions);
                free(curword);
				free(nextword);
                continue;
            }

			/*
			 * No suggestions for next word
			 */
            if (nextword_suggestions == NULL) {
                word_list *node = curword_suggestions;
				print_word_list(node, curword);
                free_word_list(curword_suggestions);
                free(prevword);
                prevword = NULL;
				free(nextword);
                continue;
            }

            char *curword_best_suggestion = curword_suggestions->word;
            char *nextword_best_suggestion = nextword_suggestions->word;
            size_t max_count = -1;
            word_list *curword_node = curword_suggestions;
            word_list *nextword_node = nextword_suggestions;
            for (; curword_node ; curword_node = curword_node->next) {
                for (; nextword_node ; nextword_node = nextword_node->next) {
                    asprintf(&bigram_word, "%s %s", curword_node->word, nextword_node->word);
                    int suggestion_frequency = spell_is_known_word(spellt, bigram_word, 2);
                    if (suggestion_frequency > max_count) {
                        max_count = suggestion_frequency;
                        curword_best_suggestion = curword_node->word;
                        nextword_best_suggestion = nextword_node->word;
                    }
                    free(bigram_word);
                    bigram_word = NULL;
                }
            }
            printf("%s: %s\n%s: %s\n", curword, curword_best_suggestion, nextword, nextword_best_suggestion);
			if (!sentence_end)
				prevword = strdup(nextword_best_suggestion);
            free_word_list(nextword_suggestions);
            free_word_list(curword_suggestions);
            continue;
        } else {
            nextword = get_next_word(inputf, &line, &sentence_end);
            if (nextword == NULL) {
                word_list *curword_suggestions = spell_get_suggestions_slow(spellt, curword, nsuggestions);
                if (curword_suggestions == NULL) {
                    free(prevword);
					free(line);
					spell_destroy(spellt);
                    return;
                }
                char *best_match = get_best_match(spellt, curword_suggestions, prevword, -1);
				if (best_match)
					printf("%s: %s\n", curword, best_match);
                free_word_list(curword_suggestions);
                free(prevword);
				free(best_match);
				free(curword);
				spell_destroy(spellt);
				free(line);
                return;
            }

            if (!spell_is_known_word(spellt, nextword, 1)) {
                word_list *suggestions = spell_get_suggestions_slow(spellt, curword, nsuggestions);
				if (suggestions) {
					char *best_match = get_best_match(spellt, suggestions, prevword, -1);
					if (best_match) {
						printf("%s: %s\n", curword, best_match);
						free(curword);
						curword = best_match;
					} else {
						free(curword);
						curword = NULL;
					}
					free_word_list(suggestions);
				}

                word_list *nextword_suggestions = spell_get_suggestions_slow(spellt, nextword, nsuggestions);
				if (nextword_suggestions) {
					char *best_match = get_best_match(spellt, nextword_suggestions, curword, -1);
					if (best_match) {
						printf("%s: %s\n", nextword, best_match);
						free(prevword);
						if (!sentence_end)
							prevword = best_match;
						else
							prevword = NULL;
					} else {
						free(prevword);
						prevword = NULL;
					}
					free_word_list(nextword_suggestions);
				}
				free(curword);
				free(nextword);
                continue;
            }

            word_list *curword_suggestions = spell_get_suggestions_slow(spellt, curword, nsuggestions);
            char *best_match = NULL;
            size_t max_product = 0;
			word_list *node = curword_suggestions;
            for(; node ; node = node->next) {
                asprintf(&bigram_word, "%s %s", prevword, node->word);
                int prevword_suggestion_frequency = spell_is_known_word(spellt, bigram_word, 2);
                free(bigram_word);
                bigram_word = NULL;
                asprintf(&bigram_word, "%s %s", node->word, nextword);
                int nextword_suggestion_frequency = spell_is_known_word(spellt, bigram_word, 2);
                int product = nextword_suggestion_frequency * prevword_suggestion_frequency;
                if (product > max_product) {
                    max_product = product;
                    best_match = node->word;
                }
                free(bigram_word);
                bigram_word = NULL;

            }
			if (best_match)
				printf("%s: %s\n", curword, best_match);
            free(prevword);
            free(curword);
			if (!sentence_end)
				prevword = nextword;
			else
				prevword = NULL;
			free_word_list(curword_suggestions);
            continue;
        }


        /* If no bigrams found, check the unigram index for this word */
        /*	if (max_index == -1) {
            word_list *suggestions2 = spell_get_suggestions_slow(spellt, word, nsuggestions);
            if (suggestions2)
            printf("%s: %s\n", word, suggestions2->word);
            free_word_list(suggestions2);
            } else
            printf("%s: %s\n", word, match);

            free_word_list(suggestions);*/

    }
    free(line);
    line = NULL;
	free(prevword);
	spell_destroy(spellt);
}


int
main(int argc, char **argv)
{
    FILE *input = stdin;
    char *whitelist_filepath = NULL;
    int ch;

    size_t nsuggestions = 10;

    while ((ch = getopt(argc, argv, "c:i:w:")) != -1) {
        switch (ch) {
            case 'c':
                nsuggestions = strtol(optarg, NULL, 10);
                break;
            case 'i':
                input = fopen(optarg, "r");
                if (input == NULL)
                    err(EXIT_FAILURE, "Failed to open %s", optarg);
                break;
            case 'w':
                whitelist_filepath = optarg;
                break;
            default:
                usage();
                break;
        }
    }

    do_bigram(input, whitelist_filepath, nsuggestions);
    if (input != stdin)
        fclose(input);
    return 0;
}
