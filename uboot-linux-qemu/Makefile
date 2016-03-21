download:
	wget ftp://ftp.denx.de/pub/u-boot/u-boot-2015.04.tar.bz2
	wget http://www.kernel.org/pub/linux/kernel/v3.0/linux-3.2.tar.bz2
	wget https://busybox.net/downloads/busybox-1.24.1.tar.bz2

extra:
	tar -jxvf u-boot-2015.04.tar.bz2
	tar -jxvf linux-3.2.tar.bz2
	tar -jxvf busybox-1.24.1.tar.bz2

prepare:
	cd linux-3.2 && make vexpress_defconfig ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
	cd busybox-1.24.1 &&  make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- defconfig
	cd u-boot-2015.04 && make vexpress_ca9x4_defconfig  ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
	cd busybox-1.24.1 &&  make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
	#select Build BusyBox as a static binary (no shared libs) 

kernel:
	cd linux-3.2 && make  ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- all
uImage: 
	cd linux-3.2 && make  ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage

busybox:
	cd busybox-1.24.1 &&  make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- install
	cd busybox-1.24.1/_install/ && mkdir -p  dev etc lib proc sys tmp mnt
	mkdir -p busybox-1.24.1/_install/etc/init.d
	cp fstab busybox-1.24.1/_install/etc/fstab
	cp rcS busybox-1.24.1/_install/etc/init.d/
	cd busybox-1.24.1/_install/ &&  find . | cpio -o --format=newc > ../rootfs.img && cd .. && gzip -c rootfs.img > rootfs.img.gz

rootfs:
	dd if=/dev/zero of=rootfs.ext3 bs=1M count=32
	sudo mkfs.ext3 rootfs.ext3
	mkdir -p rootdir
	sudo mount -t ext3 rootfs.ext3 rootdir/ -o loop
	sudo cp busybox-1.24.1/_install/* rootdir/ -r
	cd rootdir && sudo mkdir -p dev etc lib proc sys tmp mnt
	sudo cp fstab rootdir/etc/fstab
	sudo mkdir -p  rootdir/etc/init.d/
	sudo cp rcS rootdir/etc/init.d/
	sudo umount  rootdir

uboot:
	cd u-boot-2015.04/ && make all ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-

run: 
	qemu-system-arm -M vexpress-a9 -m 512M  -kernel linux-3.2/arch/arm/boot/zImage -initrd busybox-1.24.1/rootfs.img.gz -nographic   -serial stdio -append "console=ttyAMA0 root=/dev/ram mem=512M rdinit=/sbin/init"

#tftp 0x60008000 uImage; setenv bootargs 'root=/dev/mmcblk0 console=ttyAMA0'; bootm 0x60008000
run-boot:
	sudo qemu-system-arm -M vexpress-a9  -m 512M -nographic -kernel u-boot-2015.04/u-boot -net nic,vlan=0 -net tap,vlan=0,ifname=tap0  -sd rootfs.ext3
