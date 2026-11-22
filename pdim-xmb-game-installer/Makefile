# PDIM standalone PSX XMB game-shortcut installer (not part of official 1.2).
VENDOR_IRX ?= ../repairbox/irx
BUILD := build
EE_BIN := $(BUILD)/PDIM_XMB_GAME_INSTALLER.ELF
SOURCES := src/main.c src/engine.c src/art_png.c src/xmb.c
OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(SOURCES))
COMMON_IRX := iomanX fileXio ps2atad ps2fs bdm bdmfs_fatfs usbd usbmass_bd sio2man padman dvrdrv dvrfile
IRX_OBJECTS := $(addprefix $(BUILD)/,$(addsuffix _irx.o,$(COMMON_IRX))) $(BUILD)/dev9_ns_irx.o $(BUILD)/ps2hdd_psx1_irx.o
EE_OBJS := $(OBJECTS) $(BUILD)/ppaa_plan.o $(BUILD)/iso_utils.o $(BUILD)/cover_index.o $(BUILD)/execute_kelf.o $(BUILD)/osd_launcher.o $(IRX_OBJECTS)
EE_LIBS := -lfileXio -lpad -lcdvd -ldebug -lpatches -lpng -lz -lm
EE_INCS := -Iinclude -I../pdim-hdd-v1.2-installer/include -I../dvr-iso-manager/include -I$(PS2SDK)/ports/include
EE_CFLAGS := -std=gnu11 -Os -Wall -Wextra -fdata-sections -ffunction-sections
EE_LDFLAGS := -Wl,--gc-sections
.PHONY: all clean
all: $(EE_BIN)
$(BUILD)/%.o: %.c $(wildcard include/*.h)
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/ppaa_plan.o: ../pdim-hdd-v1.2-installer/src/ppaa_plan.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/iso_utils.o: ../dvr-iso-manager/src/iso_utils.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/cover_index.o: ../dvr-iso-manager/src/cover_index.c
	@mkdir -p $(dir $@)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/dev9_ns_irx.c: ../dvr-iso-manager/modules/dev9_ns.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ dev9_ns_irx
$(BUILD)/ps2hdd_psx1_irx.c: $(VENDOR_IRX)/psx1/ps2hdd-sparse-skip.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_psx1_irx
$(BUILD)/dvrdrv_irx.c: ../dvr-iso-manager/modules/dvrdrv.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ dvrdrv_irx
$(BUILD)/dvrfile_irx.c: ../dvr-iso-manager/modules/dvrfile.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ dvrfile_irx
$(BUILD)/%_irx.c: $(VENDOR_IRX)/pinned/%.irx
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
$(BUILD)/execute_kelf.c: assets/EXECUTE.ELF
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ execute_kelf
$(BUILD)/osd_launcher.c: assets/BOOT.ELF
	@mkdir -p $(dir $@)
	$(PS2SDK)/bin/bin2c $< $@ osd_launcher
$(BUILD)/%_irx.o: $(BUILD)/%_irx.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/execute_kelf.o: $(BUILD)/execute_kelf.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
$(BUILD)/osd_launcher.o: $(BUILD)/osd_launcher.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
clean:
	rm -rf $(BUILD)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
