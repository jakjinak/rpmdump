
RPMINCLUDES := ../rpm/include
RPMTAGHEADER := ${RPMINCLUDES}/rpm/rpmtag.h

.PHONY: all gen clean

all:

BIN_FILE := rpmdump
GEN_FILES := tagtypes.gen.cpp tags.gen.cpp sigtags.gen.cpp

clean:
	rm -f ${GEN_FILES}
	rm -f ${BIN_FILE}

gen: ${GEN_FILES}

tagtypes.gen.cpp: ${RPMTAGHEADER}
	rm -f "$@"
	@sed -n -e '/RPM_MIN_TYPE/,/RPM_MAX_TYPE/p' < "$<" | sed -e '1d' -e '$$d' -e 's/^[\t ]*RPM_\(.*\)_TYPE[ \t]*=[ \t]*\([0-9]*\).*/    { \2, "\1" },/' > "$@"
	@cat "$@"

tags.gen.cpp: ${RPMTAGHEADER}
	rm -f "$@"
	@sed -n -e '/typedef enum rpmTag_e {/,/} rpmTag;/p' < "$<" | sed -n -e '/NOT_FOUND/d' -e '/^[\t ]*RPMTAG_.*=/p' | sed 's/^[\t ]*RPMTAG_\([^\t ]*\).*/    { RPMTAG_\1, "\1" },/' > "$@"
	@cat "$@"

sigtags.gen.cpp: ${RPMTAGHEADER}
	rm -f "$@"
	@sed -n -e '/typedef enum rpmSigTag_e {/,/} rpmSigTag;/p' < "$<" | sed -n -e '/NOT_FOUND/d' -e '/^[\t ]*RPMSIGTAG_.*=/p' | sed 's/^[\t ]*RPMSIGTAG_\([^\t ]*\).*/    { RPMSIGTAG_\1, "\1" },/' > "$@"
	@cat "$@"

all: ${GEN_FILES} ${BIN_FILE}

${BIN_FILE}: rpmdump.cpp stringconsts.cpp
	g++ -std=c++11 -g -I${RPMINCLUDES} $^ -o "$@"
