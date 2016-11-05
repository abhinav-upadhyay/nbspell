.include <bsd.own.mk>

PROGS=			spell
SRCS.spell=		spell.c libspell.c

LDADD+= -lutil

BINDIR=		/usr/bin

dict.c: dict/web2
	( set -e; ${TOOL_NBPERF} -n dict_hash -s -p ${.ALLSRC};	\
	echo 'static const char *dict[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

DPSRCS+=	dict.c
CLEANFILES+=	dict.c

.include <bsd.prog.mk>
