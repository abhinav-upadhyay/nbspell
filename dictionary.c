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
#include<stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/queue.h>
#include <sys/rbtree.h>
#include "libspell.h"
#include "spellutils.h"

#include "websters.c"

static void
usage(void)
{
	fprintf(stderr, "dictionary [-i input] [-n ngram] [-o output]\n");
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
parse_file(FILE * f, FILE * output, long ngram)
{

	rb_tree_t words_tree;
	const rb_tree_ops_t tree_ops = {
		.rbto_compare_nodes = compare_words,
		.rbto_compare_key = compare_words,
		.rbto_node_offset = offsetof(word_count, rbtree),
		.rbto_context = NULL
	};
	rb_tree_init(&words_tree, &tree_ops);

	typedef struct entry {
		SIMPLEQ_ENTRY(entry) entries;
		char *word;
	}     entry;

	char *word = NULL;
	char *line = NULL;
	char *templine;
	char *ngram_string = NULL;
	char *temp = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;
	SIMPLEQ_HEAD(wordq, entry) head;
	struct wordq *headp;
	size_t counter = 0;
	int sentence_end = 0;
	entry *e;
	entry *np;
	entry *first;
	SIMPLEQ_INIT(&head);
	size_t linecount = 0;

	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
	//	if (++linecount >= 400000)
	//		break;
		templine = line;
		templine[bytes_read--] = 0;
		if (templine[bytes_read] == '\r')
			templine[bytes_read] = 0;
		while (*templine) {
			wordsize = strcspn(templine, ".?\'\",;-: \t");
			word = templine;
			if (word[wordsize] == '.' ||
			    word[wordsize] == '?' ||
			    word[wordsize] == ':' ||
			    word[wordsize] == '-' ||
			    word[wordsize] == ';' ||
			    word[wordsize] == '\t'
			    )
				sentence_end++;
			word[wordsize] = 0;
			templine += wordsize + 1;
			sanitized_word = sanitize_string(word);
			if (!sanitized_word || !sanitized_word[0]) {
				free(sanitized_word);
				goto clear_list;
			}
			lower(sanitized_word);
			if (!is_known_word(sanitized_word)) {
				free(sanitized_word);
				goto clear_list;
			}
			e = emalloc(sizeof(*e));
			e->word = sanitized_word;
			counter++;
			if (counter > ngram) {
				first = SIMPLEQ_FIRST(&head);
				SIMPLEQ_REMOVE_HEAD(&head, entries);
				free(first->word);
				free(first);
			}
			SIMPLEQ_INSERT_TAIL(&head, e, entries);
			if (counter < ngram)
				goto clear_list;

			SIMPLEQ_FOREACH(np, &head, entries) {
				if (ngram_string) {
					easprintf(&temp, "%s %s", ngram_string, np->word);
					free(ngram_string);
					ngram_string = temp;
					temp = NULL;
				} else {
					ngram_string = estrdup(np->word);
				}
			}

			wc.word = ngram_string;
			void *node = rb_tree_find_node(&words_tree, &wc);
			if (node == NULL) {
				wcnode = emalloc(sizeof(*wcnode));
				wcnode->word = ngram_string;
				wcnode->count = 1;
				rb_tree_insert_node(&words_tree, wcnode);
			} else {
				wcnode = (word_count *) node;
				wcnode->count++;
				free(ngram_string);
			}
			ngram_string = NULL;

	clear_list:
			if (sentence_end) {
				while ((e = SIMPLEQ_FIRST(&head)) != NULL) {
					SIMPLEQ_REMOVE_HEAD(&head, entries);
					free(e->word);
					free(e);
				}
				counter = 0;
				sentence_end = 0;
			}
		}
		free(line);
		line = NULL;
		linesize = 0;
	}


	word_count *tmp;
	RB_TREE_FOREACH(tmp, &words_tree)
	    fprintf(output, "%s\t%d\n", tmp->word, tmp->count);

	if (ngram != 1)
		return;

	/* For unigrams, the rare words which were not found in
	 * the corpus, write them with frequency of 1. Better
	 * than skipping them altogether.
	 */
	int i;
	for (i = 0; i < sizeof(dict)/sizeof(dict[0]); i++) {
		wc.word = (char *) dict[i];
		void *node = rb_tree_find_node(&words_tree, &wc);
		if (node == NULL)
			fprintf(output, "%s\t%d\n", dict[i], 1);
	}
}


int
main(int argc, char **argv)
{
	FILE *inputfile = stdin;
	FILE *outputfile = stdout;
	long ngram = 1;
	int ch;

	while ((ch = getopt(argc, argv, "i:n:o:")) != -1) {
		switch (ch) {
		case 'i':
			inputfile = fopen(optarg, "r");
			if (inputfile == NULL)
				err(EXIT_FAILURE, "Failed to open %s", optarg);
			break;
		case 'n':
			ngram = strtol(optarg, NULL, 10);
			break;
		case 'o':
			outputfile = fopen(optarg, "w");
			if (outputfile == NULL)
				err(EXIT_FAILURE, "Failed to open %s for writing", optarg);
			break;
		default:
			usage();
			break;
		}
	}
	parse_file(inputfile, outputfile, ngram);
	if (inputfile != stdin)
		fclose(inputfile);
	fclose(inputfile);
	if (outputfile != stdout)
		fclose(outputfile);
	return 0;
}
