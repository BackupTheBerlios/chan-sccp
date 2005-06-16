# Location of asterisk source files (can be /usr/src if using a sys wide include directory)
ASTERISK_SOURCE=/usr/src/asterisk/asterisk
# Location asterisk modules install to
AST_MODULES_DIR=/usr/lib/asterisk/modules/
# Asterisk version, currently only v1_0 and HEAD are supported
ASTERISK_VERSION=HEAD

PROC=$(shell uname -m)

DEBUG=-ggdb
DEFINE=-Iinclude -I$(ASTERISK_SOURCE)/include -D_REENTRANT -D_GNU_SOURCE  -O6  -DCRYPTO -fPIC -DASTERISK_VERSION_$(ASTERISK_VERSION)
CFLAG= -pipe -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAG+=$(shell if $(CC) -march=$(PROC) -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo " -march=$(PROC)"; fi)
CFLAG+=$(shell if uname -m | grep -q ppc; then echo " -fsigned-char"; fi)
#CFLAG+= -pedantic
#CFLAG+= -W
#CFLAG+= -Wpointer-arith
#CFLAG+= -Wcast-qual
#CFLAG+= -Wwrite-strings
#CFLAG+= -Wconversion
#CFLAG+= -Wredundant-decls # Useless as too many false positives from asterisk source
CFLAG+= -Wnested-externs 
CFLAG+= -Wlong-long
CFLAG+= $(DEBUG)

LINTOPTS=-linelen 1000
LINTOPTS+=+matchanyintegral
LINTOPTS+=-retvalother
LINTOPTS+=-type
LINTOPTS+=-DASTERISK_VERSION_$(ASTERISK_VERSION)

FLS+=chan_sccp
FLS+=sccp_actions
FLS+=sccp_channel
FLS+=sccp_device
FLS+=sccp_line
FLS+=sccp_helper
FLS+=sccp_pbx
FLS+=sccp_cli
FLS+=sccp_softkeys
FLS+=sccp_sched
FLS+=sccp_socket

CFLAGS+=$(DEFINE) $(CFLAG) -I$(ASTERISK_SOURCE)
HEADERS=$(shell for i in $(FLS) ; do echo $$i.h ; done)
OBJECTS=$(shell for i in $(FLS) ; do echo .tmp/$$i.o ; done)
SOURCES=$(shell for i in $(FLS) ; do echo $$i.c ; done)

chan_sccp.so: .tmp $(OBJECTS) $(HEADERS) $(SOURCES)
	@echo "Linking chan_sccp.so"
	@$(CC) -shared -Xlinker -x -o chan_sccp.so $(OBJECTS)

.tmp/%.o: $(HEADERS)
	@printf "Now compiling .... %-15s\t%s lines \n" $*.c "`wc -l <$*.c`"
	@$(CC) -I/usr/include/ $(CFLAGS) -I$(ASTERISK_SOURCE) $(DEFINE) -c $*.c -o .tmp/$*.o

install: chan_sccp.so
	@echo "Now Installing chan_sccp.so"
	@install -m 755 chan_sccp.so $(AST_MODULES_DIR);
	@if ! [ -f /etc/asterisk/sccp.conf ]; then \
		echo "Installing config file /etc/asterisk/sccp.conf"; \
		cp conf/sccp.conf /etc/asterisk/; \
	fi
	@echo "Chan_sccp is now installed"
	@echo "Remember to disable chan_skinny by adding the following"
	@echo "line to /etc/asterisk/modules.conf:"
	@echo "noload => chan_skinny.so"

clean:
	rm -rf chan_sccp.so .tmp

lint:
	@splint -weak -warnposix -varuse -fullinitblock -unrecog $(LINTOPTS) $(FLS) | tee splintlog

Makefile:

MEEP=chan_sccp.o
MOO=chan_sccp.c	

.tmp/chan_sccp.o:     chan_sccp.c
.tmp/sccp_actions.o:  sccp_actions.c
.tmp/sccp_channel.o:  sccp_channel.c
.tmp/sccp_device.o:   sccp_device.c
.tmp/sccp_helper.o:   sccp_helper.c
.tmp/sccp_pbx.o:      sccp_pbx.c
.tmp/sccp_cli.o:      sccp_cli.c
.tmp/sccp_line.o:     sccp_line.c
.tmp/sccp_softkeys.o: sccp_softkeys.c
.tmp/sccp_sched.o:    sccp_sched.c
.tmp/sccp_socket.o:   sccp_socket.c

.tmp:
	@mkdir -p .tmp

.PHONY: lint clean
