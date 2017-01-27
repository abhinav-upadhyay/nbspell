.include <bsd.own.mk>

PROGS=			dictionary spell bigspell soundex 
SRCS.spell=		spell.c libspell.c
SRCS.bigspell=		bigspell.c libspell.c
SRCS.dictionary=	dictionary.c libspell.c spellutils.c
SRCS.soundex=	soundex.c libspell.c

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
