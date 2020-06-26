VI_OBJS = \
	vi/line.o\
	vi/text.o\
	vi/help.o\
	vi/editmode.o\
	vi/vi.o\
	vi/vulib.o\
	vi/baseline.o\
	vi/cursor.o\
	vi/color.o\
	vi/re.o\
	vi/stl.o\

vi/%.o: vi/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


_vi: $(ULIB) $(VI_OBJS)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

VI_SYNTAX =\
	vi/syntax/c.vi\
	vi/syntax/html.vi\

%.vi:
	cp $(VI_SYNTAX) ./