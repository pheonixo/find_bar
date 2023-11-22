PHEONIX = /home/steven/Development/Pheonix
#-nostdinc
CFLAGS = -x c -std=gnu99 -pipe -fno-builtin -fmessage-length=0  -g -I$(PHEONIX)/libc/include \
			-I/usr/include/gtksourceview-3.0 `pkg-config --cflags gtk+-3.0`
WARNINGS = -Wall -Wextra -Wnormalized -Wuninitialized -Winit-self -Wno-unknown-pragmas
# add -Wno-deprecated-declarations for  sourceview-3.0
XTRA_WARN = -Wno-missing-braces -Wno-misleading-indentation -Wno-shift-negative-value \
			-Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-unused-parameter \
			-Wno-deprecated-declarations
LFLAGS := `pkg-config --libs gtk+-3.0` -lgtksourceview-3.0 -lrt

PFILE_SRC := \
	lci_buffer_open.c \
	$(PHEONIX)/libc/locale/codesets/lc_codeset.c \
	$(PHEONIX)/libc/locale/codesets/csBig5.c \
	$(PHEONIX)/libc/locale/codesets/csGB18030.c \
	$(PHEONIX)/libc/locale/codesets/csISO8859.c \
	$(PHEONIX)/libc/locale/codesets/csJISX0213.c \
	$(PHEONIX)/libc/locale/codesets/csKOI8.c \
	$(PHEONIX)/libc/locale/codesets/csPTCP154.c \
	$(PHEONIX)/libc/locale/codesets/csUTF.c \
	$(PHEONIX)/libc/locale/codesets/cswindows.c

PHX_SRC := \
	$(PHEONIX)/libc/errno/errno.c \
	$(PHEONIX)/libc/stdlib/realpath.c

LCI_GTK_SRC := \
	lci_treecolumn.c

LCI_SRC := \
	lci_sessions.c \
	lci_menus.c \
	lci_dialogs.c \
	lci_textport.c \
	lci_treeport.c \
	lci_findport.c \
	lci_file_info.c \
	lci_symbols.c \
	lci_parser_C.c

LCode: $(LCI_SRC) lci.h lci_findport.h libpfile.a libtreecolumn.a $(PHX_SRC)
	gcc -o LCode $(CFLAGS) $(WARNINGS) $(XTRA_WARN) $(LCI_SRC) $(PHX_SRC) $(LFLAGS) -L. -lpfile -ltreecolumn -lm

pfile: lci_buffer_open.c
	$(foreach f, $(PFILE_SRC), gcc $(CFLAGS) -c $f $(WARNINGS) $(XTRA_WARN) -o $(basename $(notdir $f)).o; )
	$(eval TMP := $(foreach f, $(PFILE_SRC), $(basename $(notdir $f)).o))
	ar rcs libpfile.a $(TMP)
	rm $(TMP)
#	gcc -o pfile lci_buffer_open.c $(PFILE_SRC) $(CFLAGS) $(WARNINGS) $(XTRA_WARN) $(LFLAGS)

treecolumn: $(LCI_GTK_SRC) lci_treecolumn.h
	gcc -c lci_treecolumn.c $(CFLAGS) $(WARNINGS) $(XTRA_WARN) $(LFLAGS) -o lci_treecolumn.o
	ar rcs libtreecolumn.a lci_treecolumn.o
	rm lci_treecolumn.o

windows:
	gcc `pkg-config --cflags gtk+-3.0` -o windows windows.c $(LFLAGS)

