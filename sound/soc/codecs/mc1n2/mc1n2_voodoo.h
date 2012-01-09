#define NAME					   "Voodoo Yamaha MC1-N2 driver"
#define VERSION								     "1"
#define HPGAIN_SIZE							       4
#define HPSP_SIZE							      32

typedef unsigned short (*volmap);

struct mc1n2_voodoo {
	struct kobject kobj;
	struct class *mc1n2_voodoo_class;
};
