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
	(void) fprintf(stderr, "Usage: wikipedia [-f test_file] [-n ngram]\n");
	exit(1);
}


static void
do_unigram(FILE *f)
{

	char *test;
	char *answer;
	char *tab;
	ssize_t bytes_read;
	spell_t *spell = spell_init("dict/unigram.txt", NULL);
	char *line = NULL;
	size_t linesize = 0;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;
	size_t linecount = 0;
	size_t corrects = 0;
	size_t wrong = 0;
	size_t failed = 0;
	size_t five = 0;
	size_t ten = 0;
	size_t twentyfive = 0;
	size_t fifty = 0;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		linecount++;
		line[--bytes_read] = 0;
		if (line[bytes_read] == '\r')
			line[bytes_read] = 0;
		test = line;
		tab = strchr(line, '\t');
		*tab = 0;
		answer = tab + 1;
		lower(answer);
		lower(test);
		if (spell_is_known_word(spell, test, 1)) {
			if (strcmp(test, answer) == 0)
				corrects++;
			else {
				wrong++;
				printf("wrong: test: %s\t correction %s\t answer: %s\n", test, test, answer);
			}
			//printf("%s\t%s\n", test, test);
			continue;
		}

		char **corrections = spell_get_suggestions(spell, test, 1);
		if (corrections == NULL) {
			failed++;
			printf("failed: test: %s\tanswer: %s\n", test, answer);
			continue;
		}

		size_t i = 0;
		int correct_found = 0;
		while(corrections[i] != NULL) {
			char *correction = corrections[i++];
			if (strcmp(correction, answer) == 0) {
				corrects++;
				correct_found = 1;
				//printf("%s\t%s\n", test, correction);
				break;
			}
		}
		if (!correct_found) {
			printf("wrong: test: %s\t correction %s\t answer: %s\n", test, corrections[0], answer);
			wrong++;
		}
		free_list(corrections);
		free(line);
		line = NULL;
	}
	free(line);
	printf("corrects: %u\nwrongs: %u\nfailed: %u\n", corrects, wrong, failed);
}

int
main(int argc, char **argv)
{
	long ngram = 1;
	FILE *input = stdin;
	int ch;

	while ((ch = getopt(argc, argv, "f:n:")) != -1) {
		switch (ch) {
		case 'f':
			input = fopen(optarg, "r");
			if (input == NULL)
				err(EXIT_FAILURE, "Failed to open %s", optarg);
			break;
		case 'n':
			ngram = strtol(optarg, NULL, 10);
			break;
		default:
			usage();
			break;
		}
	}

	if (ngram == 1)
		do_unigram(input);
	if (input != stdin)
		fclose(input);
	return 0;
}
