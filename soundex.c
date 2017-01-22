#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "libspell.h"
#include "websters.c"

int
main(int argc, char **argv)
{
	size_t i;
	char *soundex_code;
	FILE *out = fopen("dict/soundex.txt", "w");
	if (out == NULL)
		err(EXIT_FAILURE, "Failed to open soundex");

	for (i = 0; i < sizeof(dict) / sizeof(dict[0]); i++) {
		soundex_code = soundex(dict[i]);
		if (soundex_code == NULL) {
			warnx("No soundex code found for %s", dict[i++]);
			continue;
		}
		fprintf(out, "%s\t%s\n", dict[i], soundex_code);
		free(soundex_code);
	}

	fclose(out);
	return 0;
}
