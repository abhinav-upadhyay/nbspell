#include <ctype.h>
#include <err.h>
#include<stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <sys/rbtree.h>
#include "libspell.h"

static int compare_words(void *, const void *, const void *);

static char *
parse_word(char *w, size_t len)
{
	while (w && !isalpha(w[0]))
		w++;

	if (!w)
		return NULL;

	while (len && !isalpha(w[len -1]))
		w[--len] = 0;

	if (len == 0)
		return NULL;

	return w;
}

static char *
sanitize_string(char *s)
{
    size_t len = strlen(s);
    int i = 0;
    if (s[0] == '(' && s[len - 1] == ')') {
        s[len - 1] = 0;
        s++;
    }

    char *ret = malloc(len + 1);
    memset(ret, 0, len + 1);
    while(*s) {
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

        // Why bother with words which contain other characters or numerics?
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
lower(char *word)
{
	size_t i = 0;
	char c;
	while (word[i]) {
		c = tolower(word[i]);
		word[i++] = c;
	}
}

static void
parse_file(FILE *f)
{

	static rb_tree_t words_tree;
	static const rb_tree_ops_t tree_ops = {
		.rbto_compare_nodes =  compare_words,
		.rbto_compare_key = compare_words,
		.rbto_node_offset = offsetof(word_count, rbtree),
		.rbto_context = NULL
	};
	rb_tree_init(&words_tree, &tree_ops);

	char *word = NULL;
	char *line = NULL;
	size_t linesize = 0;
	size_t wordsize = 0;
	ssize_t bytes_read;
	word_count *wcnode;
	word_count wc;
	wc.count = 0;
	char *sanitized_word = NULL;


	while ((bytes_read = getline(&line, &linesize, f)) != -1) {
		line[bytes_read - 1] = 0;
		char *templine = line;
		while (*templine) {
			wordsize = strcspn(templine, "\'\",;-:. \t\u2014");
			templine[wordsize] = 0;
			word = templine;
			templine += wordsize + 1;
			sanitized_word = sanitize_string(word);
			if (!sanitized_word || !sanitized_word[0]) {
				free(sanitized_word);
				continue;
			}


			lower(sanitized_word);
			if (!is_known_word(sanitized_word)) {
				free(sanitized_word);
				continue;
			}
			wc.word = sanitized_word;
			void *node = rb_tree_find_node(&words_tree, &wc);
			if (node == NULL) {
				wcnode = emalloc(sizeof(*wcnode));
				wcnode->word = sanitized_word;
				wcnode->count = 1;
				rb_tree_insert_node(&words_tree, wcnode);
			} else {
				wcnode = (word_count *) node;
				wcnode->count++;
				free(sanitized_word);
			}
		}
	}

	FILE *out = fopen("./out", "w");
	if (out == NULL)
		err(EXIT_FAILURE, "Failed to open dictionary.out");

	word_count *tmp;
	RB_TREE_FOREACH(tmp, &words_tree) {
		fprintf(out, "%s\t%d\n", tmp->word, tmp->count);
		//free(tmp->word);
	}
	fclose(out);

}

static int
compare_words(void *context, const void *node1, const void *node2)
{
	const word_count *wc1 = (const word_count *) node1;
	const word_count *wc2 = (const word_count *) node2;

	return strcmp(wc1->word, wc2->word);
}

int
main(int argc, char **argv)
{
	char *path = argv[1];
	FILE *f = fopen(path, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "Failed to open %s", path);
	parse_file(f);
	fclose(f);
	return 0;
}
