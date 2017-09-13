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
	(void) fprintf(stderr, "Usage: spell [-c number of suggestions]  [-i input_file] [-w whitelist]\n");
	exit(1);
}


static void
do_unigram(FILE *f, const char *whitelist_filepath, size_t nsuggestions, int fast)
{

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytes_read;
	spell_t *spell = NULL;
	char *line = NULL;
	size_t linesize = 0;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;
	word_list *corrections = NULL;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[--bytes_read] = 0;
		if (line[bytes_read] == '\r')
			line[bytes_read] = 0;
		char *templine = line;
		if (spell == NULL)
			spell = spell_init("dict/unigram.txt", whitelist_filepath);
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

			if (!fast)
				corrections = spell_get_suggestions_slow(spell, sanitized_word, nsuggestions);
			else
				corrections = spell_get_suggestions_fast(spell, sanitized_word, nsuggestions);
			word_list *node = corrections;
			size_t i = 0;
			if (corrections) {
				printf("%s: ", word);
				while (node != NULL) {
				//while(corrections[i] != NULL) {
					if (i > 0)
						printf("%s", ",");
					//char *correction = corrections[i++];
					//printf("%s", correction);
					printf("%s", node->word);
					node = node->next;
					i++;
				}
				printf("\n");
			}
			free_word_list(corrections);
//			free_list(corrections);
			//if (corrections)
			//	free_word_list(corrections);
			free(sanitized_word);
		}
		free(line);
		line = NULL;
	}
    spell_destroy(spell);
	free(line);
}

int
main(int argc, char **argv)
{
	FILE *input = stdin;
	char *whitelist_filepath = NULL;
	int ch;
	size_t nsuggestions = 1;
	int fast = 0;

	while ((ch = getopt(argc, argv, "c:fi:w:")) != -1) {
		switch (ch) {
		case 'c':
			nsuggestions = strtol(optarg, NULL, 10);
			break;
		case 'f':
			fast = 1;
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

	do_unigram(input, whitelist_filepath, nsuggestions, fast);
	if (input != stdin)
		fclose(input);
	return 0;
}
