AT=@
CC=gcc 

OS=$(shell uname)
LD_FLAGS = lib/$(OS)/libuv.a 
ifeq ($(OS),Linux)
LD_FLAGS += -lpthread -lrt
endif
CFLAGS = -Iinclude/$(OS) -g

OUTPUT_DIR = out/$(OS)
TFTPC = $(OUTPUT_DIR)/tftpc
TFTPD = $(OUTPUT_DIR)/tftpd

TFTPC_O = $(OUTPUT_DIR)/tftp.o $(OUTPUT_DIR)/tftpc.o
TFTPD_O = $(OUTPUT_DIR)/tftp.o $(OUTPUT_DIR)/tftpd.o


all:$(TFTPC) $(TFTPD)

$(OUTPUT_DIR)/%.o:%.c tftp.h tftpc.h tftpd.h
	$(AT) echo [Compile $(notdir $(CC)) $<]
	$(AT) mkdir -p $(dir $@)
	$(AT) $(CC) $(CFLAGS) -c -o $@ $<

$(TFTPC): $(TFTPC_O)
	$(AT) echo [Linking  $@]
	$(AT) $(CC)   -o $@ $(TFTPC_O) $(LD_FLAGS)
	$(AT) echo 
	
$(TFTPD): $(TFTPD_O)
	$(AT) echo [Linking  $@]
	$(AT) $(CC)   -o $@ $(TFTPD_O) $(LD_FLAGS)
	$(AT) echo 

clean:
	$(AT) rm -rf $(OUTPUT_DIR)
