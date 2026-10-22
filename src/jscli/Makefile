AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

all: jscli,ffa jscliwimp,ffa

%,ffa: %.elf
	$(OBJCOPY) -O binary $< $@

%.elf: %.o
	$(LD) --section-start .text=0 -o $@ $<

%.o: %.s jscli-defs.s
	$(AS) -o $@ $<

clean:
	rm -f *.o *.elf *,ffa
