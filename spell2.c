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

#include "libspell.h"


static void
usage(void)
{
	(void) fprintf(stderr, "Usage: spell input_file\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	if (argc < 2)
		usage();

	char *input_filename = argv[1];
	//XXX: Do permission checks on the file?
	FILE *f = fopen(input_filename, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "fopen failed");

	char *word = NULL;
	size_t wordsize = 0;
	ssize_t bytesread;
	spell_t *spell = spell_init("./out");
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
			wordsize = strcspn(templine, "?\'\",;-:. \t\u2014");
			templine[wordsize] = 0;
			word = templine;
			templine += wordsize + 1;
			/*			sanitized_word = sanitize_string(word);
						if (!sanitized_word || !sanitized_word[0]) {
						free(sanitized_word);
						continue;
						}*/
			lower(word);
			if (spell_is_known_word(spell, word))
				continue;

			char **corrections = spell_get_suggestions(spell, word);
			size_t i = 0;
			while(corrections && corrections[i] != NULL) {
				char *correction = corrections[i++];
				printf("%s: %s\n", word, correction);
			}
			free_list(corrections);
		}
	}


	fclose(f);
	return 0;
}
