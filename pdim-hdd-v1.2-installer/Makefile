# PDIM 1.2 HDD pre-release installer: PP.DVRISO, hardware test required.
# Uses pinned RepairBox PSX1 HDD and USB IOP modules, with attribution.
VENDOR_IRX ?= ../repairbox/irx
BUILD := build
EE_BIN := $(BUILD)/PDIM_HDD_V12_INSTALLER.ELF
SOURCES := src/main.c src/policy.c src/installer.c src/xmb.c src/ppaa_plan.c
OBJECTS := $(patsubst src/%.c,$(BUILD)/src/%.o,$(SOURCES))
COMMON_IRX := iomanX fileXio ps2atad ps2fs bdm bdmfs_fatfs usbd usbmass_bd sio2man padman
IRX_OBJECTS := $(addprefix $(BUILD)/,$(addsuffix _irx.o,$(COMMON_IRX))) $(BUILD)/dev9_ns_irx.o $(BUILD)/ps2hdd_psx1_irx.o
# Makefile.eeglobal supplies the sole link recipe for $(EE_BIN).
EE_OBJS := $(OBJECTS) $(BUILD)/v12_net_config.o $(IRX_OBJECTS)
EE_LIBS := -lfileXio -lpad -ldebug -lpatches
EE_INCS := -Iinclude -I../dvr-iso-manager/include -I$(PS2SDK)/ports/include
EE_CFLAGS := -std=gnu11 -Os -Wall -Wextra -fdata-sections -ffunction-sections
EE_LDFLAGS := -Wl,--gc-sections
.PHONY: all clean host-test
all: $(EE_BIN)
host-test:
	cc -std=c11 -Wall -Wextra -Werror -Iinclude src/policy.c tests/test_policy.c -o /tmp/pdim-v12-policy-test
	/tmp/pdim-v12-policy-test
	cc -std=c11 -Wall -Wextra -Werror -Iinclude src/ppaa_plan.c tests/test_ppaa_plan.c -o /tmp/pdim-v12-ppaa-test
	/tmp/pdim-v12-ppaa-test
	cc -std=c11 -Wall -Wextra -Werror -I../dvr-iso-manager/include ../dvr-iso-manager/src/net/v12_net_config.c tests/test_net_config.c -o /tmp/pdim-v12-net-test
	/tmp/pdim-v12-net-test V12_NET.CNF
$(BUILD)/src/%.o: src/%.c $(wildcard include/*.h)
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
# Reuse exactly the HTTPS runtime's strict V12_NET.CNF parser.
$(BUILD)/v12_net_config.o: ../dvr-iso-manager/src/net/v12_net_config.c ../dvr-iso-manager/include/v12_net_config.h
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
# Reuse our DESR-7000-verified HDD NETFIX DEV9 rather than a second provider.
$(BUILD)/dev9_ns_irx.c: ../dvr-iso-manager/modules/dev9_ns.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ dev9_ns_irx

$(BUILD)/ps2hdd_psx1_irx.c: $(VENDOR_IRX)/psx1/ps2hdd-sparse-skip.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_psx1_irx
$(BUILD)/%_irx.c: $(VENDOR_IRX)/pinned/%.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
$(BUILD)/%_irx.o: $(BUILD)/%_irx.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
clean:
	rm -rf $(BUILD)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
