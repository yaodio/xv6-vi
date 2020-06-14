VI_OBJS = \
	vi/vi.o\
	vi/vulib.o\
	vi/baseline.o\

vi/%.o: vi/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


_vi: $(ULIB) $(VI_OBJS)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym