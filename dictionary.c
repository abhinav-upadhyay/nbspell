#define _GNU_SOURCE
#include <ctype.h>
#include <err.h>
#include<stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <util.h>

#include <sys/queue.h>
#include "rbtree.h"
#include "libspell.h"

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
parse_file(FILE * f, long ngram)
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
		if (++linecount >= 400000)
			break;
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
			e = malloc(sizeof(*e));
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
					asprintf(&temp, "%s %s", ngram_string, np->word);
					free(ngram_string);
					ngram_string = temp;
					temp = NULL;
				} else {
					ngram_string = strdup(np->word);
				}
			}

			wc.word = ngram_string;
			void *node = rb_tree_find_node(&words_tree, &wc);
			if (node == NULL) {
				wcnode = malloc(sizeof(*wcnode));
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
	}
    free(line);

	FILE *out = fopen("./out", "w");
	if (out == NULL)
		err(EXIT_FAILURE, "Failed to open dictionary.out");

	word_count *tmp;
	RB_TREE_FOREACH(tmp, &words_tree)
	    fprintf(out, "%s\t%u\n", tmp->word, tmp->count);
	fclose(out);
}


int
main(int argc, char **argv)
{
	char *path = argv[1];
	long ngram = 1;
	if (argc >= 2) {
		ngram = strtol(argv[2], NULL, 10);
	}
	FILE *f = fopen(path, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "Failed to open %s", path);
	parse_file(f, ngram);
	fclose(f);
	return 0;
}
