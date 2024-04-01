CFLAGS = -x c -std=gnu99 -pipe -fno-builtin -fmessage-length=0 -nostdinc -g -I./
WARNINGS = -Wall -Wextra -Wnormalized -Wuninitialized -Winit-self -Wno-unknown-pragmas
XTRA_WARN = -Wno-missing-braces -Wno-misleading-indentation -Wno-shift-negative-value \
			-Wno-implicit-fallthrough -Wno-maybe-uninitialized
LFLAGS := -x c -std=gnu99 -pipe -fno-builtin -fmessage-length=0 -g -I./

LIB_CTYPE := \
	pxlocale.c \
	ctype.c \
	ctype_ucs.c \
	locale/localeconv.c \
	locale/newlocale.c \
	locale/datafiles/lc_calendar.c \
	locale/datafiles/lc_format.c \
	locale/datafiles/lc_era.c \
	locale/datafiles/lc_messages.c \
	locale/datafiles/lc_monetary.c \
	locale/datafiles/lc_numeric.c \
	locale/datafiles/locale_data.c \
	locale/codesets/lc_codeset.c \
	locale/codesets/csBig5.c \
	locale/codesets/csGB18030.c \
	locale/codesets/csISO8859.c \
	locale/codesets/csJISX0213.c \
	locale/codesets/csKOI8.c \
	locale/codesets/csPTCP154.c \
	locale/codesets/csUTF.c \
	locale/codesets/cswindows.c \
	locale/rwSystems/numbers.c \
	errno.c

libctype.a: $(LIB_CTYPE) makefile
	@mkdir -p build
	$(foreach f, $(LIB_CTYPE), gcc $(LFLAGS) $(WARNINGS) $(XTRA_WARN) -c $f -o ./build/$(basename $(notdir $f)).o; )
	$(eval BASE := $(foreach f, $(LIB_CTYPE), ./build/$(basename $(notdir $f)).o))
	ar rcs ./build/$@ $(BASE)
	@rm $(BASE)
