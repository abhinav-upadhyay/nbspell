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

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "libspell.h"


static void
usage(void)
{
	(void) fprintf(stderr, "Usage: spell2 [-f input_file] [-n ngram] [-w whitelist]\n");
	exit(1);
}


static char *
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

static void
do_bigram(FILE *inputf, const char *whitelist_filepath)
{

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytesread;
	spell_t *spellt = spell_init("dict/bigram.txt", whitelist_filepath);
	char *line = NULL;
	size_t linesize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;
	char *prevword = NULL;
	int sentence_end = 0;
	char *bigram_word = NULL;
	char *correction = NULL;
	size_t i;

	while ((bytes_read = getline(&line, &linesize, inputf)) != -1) {
		line[--bytes_read] = 0;
		if (line[bytes_read] == '\r')
			line[bytes_read] = 0;
		char *templine = line;
		while (*templine) {
			if (sentence_end) {
				sentence_end = 0;
				prevword = NULL;
			}
			wordsize = strcspn(templine, "()<>@?\'\",;-:. \t");
			switch (templine[wordsize]) {
				case '?':
				case '.':
				case ';':
				case '-':
				case '\t':
				case '(':
				case ')':
					sentence_end = 1;
					break;
			}

			templine[wordsize] = 0;
			prevword = word;
			word = templine;
			templine += wordsize + 1;
			if (strlen(word) <= 1)
				continue;
			while (*templine == ' ')
				templine++;
			/*			sanitized_word = sanitize_string(word);
						if (!sanitized_word || !sanitized_word[0]) {
						free(sanitized_word);
						continue;
						}*/
			lower(word);

			if (is_known_word(word))
				continue;

			if (is_whitelisted_word(spellt, word)) {
				sentence_end = 1;
				continue;
			}

			if (correction != NULL) {
				free(correction);
				correction = NULL;
			}

			/*
			 * First word of the sentence
			 */
			if (prevword == NULL) {
				char **s = spell_get_suggestions(spellt, word, 1);
				if (s != NULL && s[0] != NULL)
					correction = estrdup(s[0]);
				free_list(s);
				if (correction != NULL)
					printf("%s: %s\n", word, correction);
				word = correction;
				continue;
			}

			char **suggestions = spell(word);
			int max_index = -1;
			size_t max_frequency = 0;
			for (i = 0; suggestions && suggestions[i]; i++) {
				easprintf(&bigram_word, "%s %s", prevword, suggestions[i]);
				int suggestion_frequency = spell_is_known_word(spellt, bigram_word, 2);
				if (suggestion_frequency > max_frequency) {
					max_frequency = suggestion_frequency;
					max_index = i;
				}
				free(bigram_word);
				bigram_word = NULL;
			}

			/* If no bigrams found, check the unigram index for this word */
			if (max_index == -1) {
				char **suggestions2 = spell_get_suggestions(spellt, word, 1);
				if (suggestions2 && suggestions2[0])
					printf("%s: %s\n", word, suggestions2[0]);
				free(suggestions2);
			} else
				printf("%s: %s\n", word, suggestions[max_index]);

			free_list(suggestions);

		}
	}
}


static void
do_unigram(FILE *f, const char *whitelist_filepath)
{

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytes_read;
	spell_t *spell = spell_init("dict/unigram.txt", whitelist_filepath);
	char *line = NULL;
	size_t linesize = 0;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[--bytes_read] = 0;
		if (line[bytes_read] == '\r')
			line[bytes_read] = 0;
		char *templine = line;
		while (*templine) {
			wordsize = strcspn(templine, " ");
			templine[wordsize] = 0;
			word = templine;
			templine += wordsize + 1;
			if (strlen(word) <= 1)
				continue;
			while (*templine == ' ')
				templine++;

			lower(word);
			sanitized_word = sanitize_string(word);
			if (!sanitized_word || !sanitized_word[0]) {
				free(sanitized_word);
				continue;
			}

			if (spell_is_known_word(spell, sanitized_word, 1)) {
				free(sanitized_word);
				continue;
			}

			if (is_whitelisted_word(spell, sanitized_word)) {
				free(sanitized_word);
				continue;
			}

			char **corrections = spell_get_suggestions(spell, sanitized_word, 1);
			size_t i = 0;
			while(corrections && corrections[i] != NULL) {
				char *correction = corrections[i++];
				printf("%s: %s\n", word, correction);
			}
			free_list(corrections);
			free(sanitized_word);
		}
		free(line);
		line = NULL;
	}
	free(line);
}

int
main(int argc, char **argv)
{
	long ngram = 1;
	FILE *input = stdin;
	char *whitelist_filepath = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "f:n:w:")) != -1) {
		switch (ch) {
		case 'f':
			input = fopen(optarg, "r");
			if (input == NULL)
				err(EXIT_FAILURE, "Failed to open %s", optarg);
			break;
		case 'n':
			ngram = strtol(optarg, NULL, 10);
			break;
		case 'w':
			whitelist_filepath = optarg;
			break;
		default:
			usage();
			break;
		}
	}

	if (ngram == 1)
		do_unigram(input, whitelist_filepath);
	if (ngram == 2)
		do_bigram(input, whitelist_filepath);
	if (input != stdin)
		fclose(input);
	return 0;
}
