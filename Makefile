.include <bsd.own.mk>

MAN.spell=		# none
MAN.bigspell=		# none
MAN.dictionary=		# none
MAN.soundex=		# none
MAN.trie_test=		# none

PROGS=			dictionary spell bigspell soundex trie_test metaphone
SRCS.spell=		spell.c libspell.c trie.c look.c
SRCS.bigspell=		bigspell.c libspell.c trie.c look.c
SRCS.dictionary=	dictionary.c libspell.c spellutils.c trie.c look.c
SRCS.soundex=	soundex.c libspell.c trie.c look.c
SRCS.trie_test=	trie_test.c trie.c
SRCS.metaphone=	metaphone.c libspell.c trie.c look.c

LDADD+= -lutil

BINDIR=		/usr/bin

websters.c: dict/web3
	( set -e; ${TOOL_NBPERF} -n dict_hash -s -p ${.ALLSRC};	\
	echo 'static const char *dict[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

DPSRCS+=	websters.c
CLEANFILES+=	websters.c

.include <bsd.prog.mk>
