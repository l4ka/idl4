SUBDIRS = lib/ booter/ memsvr/ namesvr/ ext2fs/ shell/ tester/ example/

include Makeconf

ifeq ($(HAZELNUT), )
all::
	@echo "Please edit the file Makeconf.local"
else
all: $(SUBDIRS)
endif

ifneq ($(TFTPDIR), )
install::
	cp $(TOPDIR)bin/ramdisk $(TFTPDIR)
endif
