ifeq ($(CONFIG_TARGET_LOCALE_NA),y)
snd-soc-mc1n2-objs := mc1n2_SPR.o mc1n2_dbg.o \
	mcdevif.o mcdevprof.o mcdriver.o mcmachdep.o mcpacking.o mcresctrl.o mcservice.o mcdebuglog.o mcdriver_AA.o
else
snd-soc-mc1n2-objs := mc1n2.o mc1n2_dbg.o \
	mcdevif.o mcdevprof.o mcdriver.o mcmachdep.o mcpacking.o mcresctrl.o mcservice.o mcdebuglog.o mcdriver_AA.o
endif

obj-m := mc1n2_voodoo.o
obj-$(CONFIG_SND_SOC_MC1N2) += snd-soc-mc1n2.o

