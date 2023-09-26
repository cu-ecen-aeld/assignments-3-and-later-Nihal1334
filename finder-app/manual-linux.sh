#!/bin/bash
# Script outline to install and build kernel.
# Author: Nihal Gupta

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
export SCRIPT_PATH=$(pwd)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    #Clean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    #build linux
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
fi

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

#Create the rootfs directory if it doesnt exist
if [ ! -d "${OUTDIR}/rootfs" ]; then
    mkdir -p "${OUTDIR}/rootfs"
fi

# TODO: Create necessary base directories

mkdir -p rootfs/bin rootfs/dev rootfs/etc rootfs/home rootfs/lib rootfs/lib64 rootfs/proc rootfs/sbin rootfs/sys rootfs/usr rootfs/var
mkdir -p rootfs/usr/bin rootfs/usr/lib rootfs/usr/sbin
mkdir -p rootfs/var/log

echo "Rootfs file created"

# List of directories to create
#directories=("bin" "dev" "etc" "home" "conf" "lib" "lib64" "proc" "sbin" "sys" "lib/modules/${KERNEL_VERSION}" "tmp" "usr/bin" "usr/sbin" "var/log")

ROOTFS="${OUTDIR}/rootfs"

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox

    make distclean
    make defconfig

else
    cd busybox
fi

# TODO: Make and install busybox

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ..
cd rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs

SYSROOT=`${CROSS_COMPILE}gcc --print-sysroot`

cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 lib/
cp ${SYSROOT}/lib64/libm.so.6 lib64/
cp ${SYSROOT}/lib64/libresolv.so.2 lib64/
cp ${SYSROOT}/lib64/libc.so.6 lib64/

# TODO: Make device nodes

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 660 dev/console c 5 1

# TODO: Clean and build the writer utility
mkdir -p $OUTDIR/rootfs/home/conf

cd $SCRIPT_PATH
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

#cd $ROOTFS
#cp writer ${OUTDIR}/rootfs/home
#cp finder.sh ${ROOTFS}/home/
#cp finder-test.sh ${ROOTFS}/home/

#mkdir home/conf
#cp $SCRIPT_PATH/conf/username.txt ${ROOTFS}/home/conf/
#cp $SCRIPT_PATH/conf/assignment.txt ${ROOTFS}/home/conf/
#cp $SCRIPT_PATH/autorun-qemu.sh ${ROOTFS}/home/
cp writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cd conf
cp username.txt ${OUTDIR}/rootfs/home
cp assignment.txt ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
#make clean
#make writer

#cp writer ${OUTDIR}/rootfs/home/


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
#cp writer ${OUTDIR}/rootfs/home/
#cp finder.sh ${OUTDIR}/rootfs/home/
#cp finder-test.sh ${OUTDIR}/rootfs/home/
#cp autorun-qemu.sh ${OUTDIR}/rootfs/home/
#cp --parents conf/username.txt ${OUTDIR}/rootfs/home/
#cp --parents conf/assignment.txt ${OUTDIR}/rootfs/home/

#cd ..
cd ${OUTDIR}/rootfs
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
cd ${ROOTFS}
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

cd ${OUTDIR}
gzip -f initramfs.cpio

cd "$OUTDIR"

echo "SCRIPT COMPLETE"
