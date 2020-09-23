VI_OBJS = \
	vi/baseline.o\
	vi/color.o\
	vi/cursor.o\
	vi/editmode.o\
	vi/help.o\
	vi/line.o\
	vi/re.o\
	vi/stl.o\
	vi/text.o\
	vi/vi.o\
	vi/vulib.o\

vi/%.o: vi/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


_vi: $(ULIB) $(VI_OBJS)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

VI_SYNTAX =\
	vi/syntax/c.vi\
	vi/syntax/html.vi\

VI_TEST = \
	vi/tests/test.c\

# mkfs assert that files dont contain "/", so we have to copy them to the upper dir
# it's stupid, you can make change to mkfs.c to avoid this
# we will be glad to see your pr
%.vi:
	cp $(VI_SYNTAX) ./
	cp $(VI_TEST) ./