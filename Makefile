.include <bsd.own.mk>

PROGS=			spell dictionary
SRCS.spell=		spell.c libspell.c
SRCS.dictionary=	dictionary.c libspell.c

LDADD+= -lutil

BINDIR=		/usr/bin

websters.c: dict/web2
	( set -e; ${TOOL_NBPERF} -n dict_hash -s -p ${.ALLSRC};	\
	echo 'static const char *dict[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

DPSRCS+=	websters.c
CLEANFILES+=	websters.c

.include <bsd.prog.mk>
