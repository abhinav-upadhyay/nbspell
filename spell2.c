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
#include <util.h>

#include "libspell.h"


static void
usage(void)
{
	(void) fprintf(stderr, "Usage: spell input_file\n");
	exit(1);
}

static void
do_bigram(const char *input_filename)
{
	//XXX: Do permission checks on the file?
	FILE *f = fopen(input_filename, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "fopen failed");

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytesread;
	spell_t *spellt = spell_init("dict/bigram.txt");
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

	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
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

			if (correction != NULL) {
				free(correction);
				correction = NULL;
			}

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

			char **suggestions = spell( word);
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
			if (max_index == -1) {
				char **suggestions2 = spell_get_suggestions(spellt, word, 1);
				if (suggestions2 && suggestions2[0])
					printf("%s %s: %s\n", prevword, word, suggestions2[0]);
				free(suggestions2);
			} else
				printf("%s %s: %s\n", prevword, word, suggestions[max_index]);

			free_list(suggestions);

		}
	}

	fclose(f);
}


static void
do_unigram(const char *input_filename)
{
	//XXX: Do permission checks on the file?
	FILE *f = fopen(input_filename, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "fopen failed");

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytesread;
	spell_t *spell = spell_init("dict/unigram.txt");
	char *line = NULL;
	size_t linesize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		while (*templine) {
			wordsize = strcspn(templine, "()<>@?\'\",;-:. \t");
			templine[wordsize] = 0;
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
			if (spell_is_known_word(spell, word, 1))
				continue;

			char **corrections = spell_get_suggestions(spell, word, 1);
			size_t i = 0;
			while(corrections && corrections[i] != NULL) {
				char *correction = corrections[i++];
				printf("%s: %s\n", word, correction);
			}
			free_list(corrections);
		}
	}


	fclose(f);
}

int
main(int argc, char **argv)
{
	if (argc < 2)
		usage();

	long ngram = 1;
	char *input_filename = argv[1];
	if (argc > 2)
		ngram = strtol(argv[2], NULL, 10);
	if (ngram == 1)
		do_unigram(input_filename);
	if (ngram == 2)
		do_bigram(input_filename);
	return 0;
}
