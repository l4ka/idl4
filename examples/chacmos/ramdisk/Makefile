CONTENT   = ext2fs example
CONTENT_F = $(addprefix $(TOPDIR)bin/, $(CONTENT))

include ../Makeconf

all: $(TOPDIR)bin/ramdisk

$(TOPDIR)bin/ramdisk: message $(TOPDIR)bin/tester $(CONTENT_F)
	dd if=/dev/zero of=$@ bs=1k count=96
	losetup /dev/loop0 $@
	mkfs -t ext2 /dev/loop0 96
	mkdir ./mntpoint
	mount /dev/loop0 ./mntpoint -t ext2
	cp message $(TOPDIR)bin/tester ./mntpoint/
	mkdir ./mntpoint/server
	cp $(CONTENT_F) ./mntpoint/server/
	umount /dev/loop0
	rmdir ./mntpoint
	losetup -d /dev/loop0

$(TOPDIR)bin/%::
	make -C $(TOPDIR)$(*F)

clean::
	rm -f $(TOPDIR)bin/ramdisk