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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct next {
	char pri[2];
	char sec[2];
	size_t offset;
} next;

typedef struct dmetaphone {
	char *pri;
	char *sec;
} dmetaphone;

static int
is_vowel(char c)
{
	if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'Y')
		return 1;
	return 0;
}

static char *
to_upper(const char *s)
{
	char *u = strdup(s);
	size_t i = 0;
	while(u[i] != 0) {
		u[i] = toupper((int) u[i]);
		i++;
	}
	return u;
}

static char *
pad(const char *s)
{
	char *ret = NULL;
	asprintf(&ret, "--%s------", s);
	if (ret == NULL)
		err(EXIT_FAILURE, "malloc failed");
	return ret;
}

static int
is_slavo_germanic(const char *s)
{
	char prev_ch = s[0];
	const char *d = s + 1;
	while (*d) {
		if (*d == 'W')
			return 1;
		if (*d == 'K')
			return 1;
		if (prev_ch == 'C' && *d == 'Z')
			return 1;
		prev_ch = *d;
		d++;
	}
	return 0;
}

static int
is_in(const char *s, size_t len, size_t count, ...)
{
	va_list ap;
	va_start(ap, count);
	size_t i;
	for (i = 0; i < count; i++)
		if (strncmp(s, va_arg(ap, char *), len) == 0)
			return 1;
	return 0;
}

dmetaphone
double_metaphone(const char *s)
{
	char *st = to_upper(s);
	int is_sl_germanic = is_slavo_germanic(s);
	size_t len = strlen(st);
	size_t first = 2;
	size_t last = first + len - 1;
	size_t pos = first;
	char *pri = malloc(len);
	char *sec = malloc(len);
	size_t pri_offset = 0;
	size_t sec_offset = 0;
	struct next nxt;
	char *padded = pad(st);
	free(st);
	st = padded;

	if ((st[first] == 'G' && st[first + 1] == 'N') ||
	    (st[first] == 'K' && st[first + 1] == 'N') ||
	    (st[first] == 'P' && st[first + 1] == 'N') ||
	    (st[first] == 'W' && st[first + 1] == 'R') ||
	    (st[first] == 'P' && st[first + 1] == 'S')
	    )
		pos++;

	if (st[first] == 'X') {
		pri[pri_offset++] = 'S';
		pos++;
	}
	while (pos <= last) {
		char ch = st[pos];
		nxt.pri[0] = 0;
		nxt.pri[1] = 0;
		nxt.sec[0] = 0;
		nxt.sec[1] = 0;
		nxt.offset = 1;
		if (is_vowel(ch)) {
			nxt.pri[0] = 0;
			nxt.pri[1] = 0;
			nxt.offset = 1;
			if (pos == first) {
				nxt.pri[0] = 'A';
				nxt.offset = 1;
			}
		} else if (ch == 'B') {
			if (st[pos + 1] == 'B') {
				nxt.pri[0] = 'P';
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'P';
				nxt.offset = 1;
			}
		} else if (ch == 'C') {
			if (pos > first + 1 && !is_vowel(st[pos - 2]) &&
			    strncmp(&st[pos - 1], "ACH", 3) &&
			    st[pos + 2] != 'I' &&
			    (st[pos + 2] != 'E' || is_in(&st[pos - 2], 6, 2, "BACHER", "MACHER"))) {
				nxt.pri[0] = 'K';
				nxt.offset = 2;
			} else if (pos == first && strncmp(&st[first], "CAESAR", 6) == 0) {
				nxt.pri[0] = 'S';
				nxt.offset = 2;
			} else if (strncmp(&st[pos], "CHIA", 4) == 0) {
				nxt.pri[0] = 'K';
				nxt.offset = 2;
			} else if (strncmp(&st[pos], "CH", 2) == 0) {
				if (pos > first && strncmp(&st[pos], "CHAE", 4) == 0) {
					nxt.pri[0] = 'K';
					nxt.sec[0] = 'X';
					nxt.offset = 2;
				} else if (pos == first && (
						is_in(&st[pos + 1], 5, 2, "HARAC", "HARIS") ||
						is_in(&st[pos + 1], 3, 4, "HOR", "HYM", "HIA", "HEM")
				    ) && strncmp(&st[first], "CHORE", 5) == 1) {
					nxt.pri[0] = 'K';
					nxt.offset = 2;
				} else if (is_in(&st[first], 4, 2, "VAN ", "VON ") ||
					    strncmp(&st[first], "SCH", 3) == 0 ||
					    is_in(&st[pos - 2], 6, 3, "ORCHES", "ARCHIT", "ORCHID") ||
					    (st[pos + 2] == 'T' ||
						st[pos + 2] == 'S') ||
					    ((is_in(&st[pos - 1], 1, 4, "A", "O", "U", "E") ||
						    pos == first) &&
					is_in(&st[pos + 2], 1, 9, "L", "R", "N", "M", "B", "H", "F", "V", "W"))) {
					nxt.pri[0] = 'A';
					nxt.offset = 2;
				} else {
					if (pos > first) {
						if (st[first] == 'M' && st[first + 1] == 'C') {
							nxt.pri[0] = 'K';
							nxt.offset = 2;
						} else {
							nxt.pri[0] = 'X';
							nxt.sec[0] = 'K';
							nxt.offset = 2;
						}
					} else {
						nxt.pri[0] = 'X';
						nxt.offset = 2;
					}
				}
			} else if (strncmp(&st[pos + 2], "CZ", 2) == 0 && strncmp(&st[pos - 2], "WICZ", 4) == 0) {
				nxt.pri[0] = 'S';
				nxt.sec[0] = 'X';
				nxt.offset = 2;
			} else if (strncmp(&st[pos + 1], "CIA", 3) == 0) {
				nxt.pri[0] = 'X';
				nxt.offset = 3;
			} else if (strncmp(&st[pos], "CC", 2) == 0 && !(pos == first + 1 && st[first] == 'M')) {
				if (is_in(&st[pos + 2], 1, 3, "I", "E", "H") && strncmp(&st[pos + 2], "HU", 2) == 1) {
					if ((pos == first + 1 && st[first] == 'A') || is_in(&st[pos - 1], 5, 2, "UCCEE", "UCCES")) {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 'S';
						nxt.offset = 3;
					} else {
						nxt.pri[0] = 'X';
						nxt.pri[1] = 0;
						nxt.offset = 3;
					}
				} else {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 2, 3, "CK", "CG", "CQ")) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (is_in(&st[pos], 2, 3, "CI", "CE", "CY")) {
				if (is_in(&st[pos], 3, 3, "CIO", "CIE", "CIA")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'X';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'S';
					nxt.offset = 2;
				}
			} else {
				if (is_in(&st[pos + 1], 2, 3, " C", " Q", " G")) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					if (is_in(&st[pos + 1], 1, 3, "C", "K", "Q") && !is_in(&st[pos + 1], 2, 2, "CE", "CI")) {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						nxt.pri[0] = 'K';
						nxt.pri[1] = 0;
						nxt.offset = 1;
					}
				}
			}
		} else if (ch == 'D') {
			if (strncmp(&st[pos], "DG", 2) == 0) {
				if (st[pos + 2] == 'I' || st[pos + 2] == 'E' || st[pos + 2] == 'Y') {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'T';
					nxt.pri[1] = 'K';
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 2, 2, "DT", "DD")) {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'F') {
			if (st[pos + 1] = 'F') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'G') {
			if (st[pos + 1] == 'H') {
				if (pos > first && !is_vowel(st[pos - 1])) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else if (pos < first + 3) {
					if (pos == first) {
						if (st[pos + 2] == 'I') {
							nxt.pri[0] = 'J';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						} else {
							nxt.pri[0] = 'K';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						}
					}
				} else if ((pos > first + 1 && is_in(&st[pos - 2], 1, 3, "B", "H", "D")) ||
					    (pos > first + 2 && is_in(&st[pos - 3], 1, 3, "B", "H", "D")) ||
				    (pos > first + 3 && is_in(&st[pos - 3], 1, 2, "B", "H"))) {
					nxt.pri[0] = 0;
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					if (pos > first + 2 && st[pos - 1] == 'U' &&
					    is_in(&st[pos - 3], 1, 5, "C", "G", "L", "R", "T")) {
						nxt.pri[0] = 'F';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						if (pos > first && st[pos - 1] != 'I') {
							nxt.pri[0] = 'K';
							nxt.pri[1] = 0;
							nxt.offset = 2;
						}
					}
				}
			} else if (st[pos + 1] == 'N') {
				if (pos == first + 1 && is_vowel(st[first]) && !is_sl_germanic) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 'N';
					nxt.sec[0] = 'N';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 'N';
					nxt.offset = 2;
				}
			} else if (strncmp(&st[pos + 1], "LI", 2) == 0 && !is_sl_germanic) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 'L';
				nxt.sec[0] = 'L';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if (pos == first && (st[pos + 1] == 'Y' ||
					is_in(&st[pos + 1], 2, 11, "ES", "EP", "EB", "EL",
				    "EY", "IB", "IL", "IN", "IE", "EI", "ER"))) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'J';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if ((strncmp(&st[pos + 1], "ER", 2) == 0 || st[pos + 1] == 'Y') &&
				    !is_in(&st[first], 6, 3, "DANGER", "RANGER", "MANGER") &&
				    !is_in(&st[pos - 1], 1, 2, "E", "I") &&
			    !is_in(&st[pos - 1], 3, 2, "RGY", "OGY")) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'J';
				nxt.sec[1] = 0;
				nxt.offset = 2;
			} else if (is_in(&st[pos + 1], 1, 3, "E", "I", "Y") ||
			    is_in(&st[pos - 1], 4, 2, "AGGI", "OGGI")) {
				if (is_in(&st[first], 4, 2, "VON ", "VAN ") ||
				    strncmp(&st[first], "SCH", 3) == 0 ||
				    strncmp(&st[pos + 1], "ET", 2) == 0) {
					nxt.pri[0] = 'K';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					if (strncmp(&st[pos + 1], "IER ", 4) == 0) {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.offset = 2;
					} else {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.sec[0] = 'K';
						nxt.sec[1] = 0;
						nxt.offset = 2;
					}
				}
			} else if (st[pos + 1] == 'G') {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'H') {
			if ((pos == first || is_vowel(st[pos - 1])) &&
			    is_vowel(st[pos + 1])) {
				nxt.pri[0] = 'H';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 2;
			}
		} else if (ch == 'J') {
			if (strncmp(&st[pos], "JOSE", 4) == 0 ||
			    strncmp(&st[first], "SAN ", 4) == 0) {
				if ((pos == first && st[pos + 4] == ' ') ||
				    strncmp(&st[first], "SAN ", 4) == 0) {
					nxt.pri[0] = 'H';
					nxt.pri[1] = 0;
				} else {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'H';
					nxt.sec[1] = 0;
				}
			} else if (pos == first && strncmp(&st[pos], "JOSE", 4) == 1) {
				nxt.pri[0] = 'J';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'A';
				nxt.sec[1] = 0;
			} else {
				if (is_vowel(st[pos - 1]) && !is_sl_germanic &&
				    is_in(&st[pos + 1], 1, 2, "A", "O")) {
					nxt.pri[0] = 'J';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'H';
					nxt.sec[1] = 0;
				} else {
					if (pos == last) {
						nxt.pri[0] = 'J';
						nxt.pri[1] = 0;
						nxt.sec[0] = ' ';
						nxt.sec[1] = 0;
					} else {
						if (!is_in(&st[pos + 1], 1, 8, "L", "T", "K", "S", "N",
							"M", "B", "Z") &&
						    !is_in(&st[pos - 1], 1, 3, "S", "K", "L")) {
							nxt.pri[0] = 'J';
							nxt.pri[1] = 0;
						} else {
							nxt.pri[0] = 0;
							nxt.pri[1] = 0;
						}
					}
				}
			}
			if (st[pos + 1] == 'J') {
				nxt.offset = 2;
			} else {
				nxt.offset = 1;
			}
		} else if (ch == 'K') {
			if (st[pos + 1] == 'L') {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'L') {
			if (st[pos + 1] == 'L') {
				if ((pos == last - 2 &&
					is_in(&st[pos - 1], 4, 3, "ILLO", "ILLA", "ALLE")) ||
				    ((is_in(&st[last - 1], 2, 2, "AS", "OS") ||
					    st[last] == 'A' || st[last] == 'O') &&
					strncmp(&st[pos - 1], "ALLE", 4) == 0)) {
					nxt.pri[0] = 'L';
					nxt.pri[1] = 0;
					nxt.sec[0] = ' ';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'L';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else {
				nxt.pri[0] = 'L';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'M') {
			if ((strncmp(&st[pos + 1], "UMB", 3) == 0 &&
				(pos + 1 == last ||
				    strncmp(&st[pos + 2], "ER", 2) == 0)) ||
			    st[pos + 1] == 'M') {
				nxt.pri[0] = 'M';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'M';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'N') {
			if (st[pos + 1] == 'N') {
				nxt.pri[0] = 'N';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'N';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'P') {
			if (st[pos + 1] == 'H') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (st[pos + 1] == 'P' || st[pos + 1] == 'B') {
				nxt.pri[0] = 'P';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'P';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'Q') {
			if (st[pos + 1] == 'Q') {
				nxt.pri[0] = 'Q';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'Q';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'R') {
			if (pos == last && !is_sl_germanic &&
			    strncmp(&st[pos - 2], "IE", 2) == 0 &&
			    !is_in(&st[pos - 4], 2, 2, "ME", "MA")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.sec[0] = 'R';
				nxt.sec[1] = 0;
			} else {
				nxt.pri[0] = 'R';
				nxt.pri[1] = 0;
			}
			if (st[pos + 1] == 'R')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		} else if (ch == 'S') {
			if (is_in(&st[pos - 1], 3, 2, "ISL", "YSL")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			} else if (pos == first && strncmp(&st[first], "SUGAR", 5) == 0) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'S';
				nxt.sec[1] = 0;
				nxt.offset = 1;
			} else if (strncmp(&st[pos], "SH", 2) == 0) {
				if (is_in(&st[pos + 1], 4, 4, "HEIM", "HOEK", "HOLM", "HOLZ")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = 'X';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				}
			} else if (is_in(&st[pos], 3, 2, "SIO", "SIA") ||
			    strncmp(&st[pos], "SIAN", 4) == 0) {
				if (!is_sl_germanic) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'X';
					nxt.sec[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				}
			} else if ((pos == first &&
					is_in(&st[pos + 1], 1, 4, "M", "N", "L", "W")) ||
			    st[pos + 1] == 'Z') {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'X';
				nxt.sec[1] = 0;
				if (st[pos + 1] == 'Z')
					nxt.offset = 2;
				else
					nxt.offset = 1;
			} else if (strncmp(&st[pos], "SC", 2) == 0) {
				if (st[pos + 2] == 'H') {
					if (is_in(&st[pos + 3], 2, 6, "OO", "ER", "EN", "UY", "ED", "EM")) {
						if (is_in(&st[pos + 3], 2, 2, "ER", "EN")) {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.sec[0] = 'S';
							nxt.sec[1] = 'K';
							nxt.offset = 3;
						} else {
							nxt.pri[0] = 'S';
							nxt.pri[1] = 'K';
							nxt.offset = 3;
						}
					} else {
						if (pos == first && !is_vowel(st[first + 3]) &&
						    st[first + 3] != 'W') {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.sec[0] = 'S';
							nxt.sec[1] = 0;
							nxt.offset = 3;
						} else {
							nxt.pri[0] = 'X';
							nxt.pri[1] = 0;
							nxt.offset = 3;
						}
					}
				} else if (is_in(&st[pos + 2], 1, 3, "I", "E", "Y")) {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 0;
					nxt.offset = 3;
				} else {
					nxt.pri[0] = 'S';
					nxt.pri[1] = 'K';
					nxt.offset = 3;
				}
			} else if (pos == last && is_in(&st[pos - 2], 2, 2, "AI", "OI")) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			} else {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				if (st[pos + 1] == 'S' || st[pos + 1] == 'Z')
					nxt.offset = 2;
				else
					nxt.offset = 1;
			}
		} else if (ch == 'T') {
			if (strncmp(&st[pos], "TION", 4) == 0) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.offset = 3;
			} else if (is_in(&st[pos], 3, 2, "TIA", "TCH")) {
				nxt.pri[0] = 'X';
				nxt.pri[1] = 0;
				nxt.offset = 3;
			} else if (strncmp(&st[pos], "TH", 2) == 0 ||
			    strncmp(&st[pos], "TTH", 3) == 0) {
				if (is_in(&st[pos + 2], 2, 2, "OM", "AM") ||
				    is_in(&st[first], 4, 2, "VON ", "VAN ") ||
				    strncmp(&st[first], "SCH", 3) == 0) {
					nxt.pri[0] = 'T';
					nxt.pri[1] = 0;
					nxt.offset = 2;
				} else {
					nxt.pri[0] = '0';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'T';
					nxt.sec[1] = 0;
					nxt.offset = 2;
				}
			} else if (st[pos + 1] == 'T' || st[pos + 1] == 'D') {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'V') {
			if (st[pos + 1] == 'V') {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else {
				nxt.pri[0] = 'F';
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'W') {
			if (strncmp(&st[pos], "WR", 2) == 0) {
				nxt.pri[0] = 'R';
				nxt.pri[1] = 0;
				nxt.offset = 2;
			} else if (pos == first && (is_vowel(st[pos + 1]) ||
				strncmp(&st[pos], "WH", 2) == 0)) {
				if (is_vowel(st[pos + 1])) {
					nxt.pri[0] = 'A';
					nxt.pri[1] = 0;
					nxt.sec[0] = 'F';
					nxt.sec[1] = 0;
					nxt.offset = 1;
				} else {
					nxt.pri[0] = 'A';
					nxt.pri[1] = 0;
					nxt.offset = 1;
				}
			} else if ((pos == last && is_vowel(st[pos - 1])) ||
				    is_in(&st[pos - 1], 5, 4, "EWSKI", "EWSKY", "OWSKI", "OWSKY") ||
			    strncmp(&st[first], "SCH", 3) == 0) {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.sec[0] = 'F';
				nxt.sec[1] = 0;
				nxt.offset = 1;
			} else if (is_in(&st[pos], 4, 2, "WICZ", "WITZ")) {
				nxt.pri[0] = 'T';
				nxt.pri[1] = 'S';
				nxt.sec[0] = 'F';
				nxt.sec[1] = 'X';
				nxt.offset = 4;
			} else {
				nxt.pri[0] = 0;
				nxt.pri[1] = 0;
				nxt.offset = 1;
			}
		} else if (ch == 'X') {
			nxt.pri[0] = 0;
			nxt.pri[1] = 0;
			if (!(pos == last && (is_in(&st[pos - 3], 3, 2, "IAU", "EAU") ||
				    is_in(&st[pos - 2], 2, 2, "AU", "OU")))) {
				nxt.pri[0] = 'K';
				nxt.pri[1] = 'S';
			}
			if (st[pos + 1] == 'C' || st[pos + 1] == 'X')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		} else if (ch == 'Z') {
			if (st[pos + 1] == 'H') {
				nxt.pri[0] = 'J';
				nxt.pri[1] = 0;
			} else if (is_in(&st[pos + 1], 2, 3, "ZO", "ZI", "ZA") ||
			    (is_sl_germanic && pos > first && st[pos - 1] != 'T')) {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
				nxt.sec[0] = 'T';
				nxt.sec[1] = 'S';
			} else {
				nxt.pri[0] = 'S';
				nxt.pri[1] = 0;
			}
			if (st[pos + 1] == 'Z' || st[pos + 1] == 'H')
				nxt.offset = 2;
			else
				nxt.offset = 1;
		}
		if (nxt.sec[0] == 0) {
			if (nxt.pri[0]) {
				pri[pri_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					pri[pri_offset++] = nxt.pri[1];
				sec[sec_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					sec[sec_offset++] = nxt.pri[1];

			}
			pos += nxt.offset;
		} else {
			if (nxt.pri[0]) {
				pri[pri_offset++] = nxt.pri[0];
				if (nxt.pri[1])
					pri[pri_offset++] = nxt.pri[1];
			}
			if (nxt.sec[0])
				sec[sec_offset++] = nxt.sec[0];
			if (nxt.sec[1])
				sec[sec_offset++] = nxt.sec[1];
			pos += nxt.offset;
		}
	}
	pri[pri_offset] = 0;
	sec[sec_offset] = 0;
	dmetaphone dm = {pri, sec};
	return dm;
}

int
main(int argc, char **argv)
{
	char *s = argv[1];
	dmetaphone dm  = double_metaphone(s);
	printf("%s\n", dm.pri);
	if (dm.sec)
		printf("%s\n", dm.sec);
	free(dm.pri);
	free(dm.sec);
	return 0;
}
