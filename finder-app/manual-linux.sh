#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

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
    # Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    cp arch/${ARCH}/boot/Image ${OUTDIR}/
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p rootfs/{bin,sbin,etc,proc,sys,var,usr/{bin,sbin},lib,lib64,dev,home,tmp}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi
# Make and install busybox
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter" 
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

# Add library dependencies to rootfs

REQUIRED_LIBS=$(${CROSS_COMPILE}readelf -a busybox | grep NEEDED | awk '{print $NF}' | tr -d '[]')
REQUIRED_LIBS_CPP=$(${CROSS_COMPILE}readelf -a ${FINDER_APP_DIR}/writer | grep NEEDED | awk '{print $NF}' | tr -d '[]')

# Append to REQUIRED_LIBS
REQUIRED_LIBS="${REQUIRED_LIBS} ${REQUIRED_LIBS_CPP}"

SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

for lib in $REQUIRED_LIBS; do
    FOUND=$(find ${SYSROOT} -name $lib | head -n 1)

    if [ -z "$FOUND" ]; then
        echo "Warning: Could not find $lib in sysroot"
        continue
    fi
    if [[ $FOUND == *lib64* ]]; then
        cp $FOUND ${OUTDIR}/rootfs/lib64/
    else
        cp $FOUND ${OUTDIR}/rootfs/lib/
    fi
done

# Copy the interpreter
INTERP=$(${CROSS_COMPILE}readelf -a busybox | grep "program interpreter" | cut -d: -f2 | tr -d '[] ')
INTERP_PATH=$(find ${SYSROOT} -name $(basename $INTERP) | head -n 1)
cp $INTERP_PATH ${OUTDIR}/rootfs/lib/

# Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 622 ${OUTDIR}/rootfs/dev/console c 5 1

# Clean and build the writer utility
make clean
make -C ${FINDER_APP_DIR} CROSS_COMPILE=${CROSS_COMPILE} VERBOSE=1
# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
mkdir -p ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -rL ${FINDER_APP_DIR}/conf ${OUTDIR}/rootfs/home/

chmod +x ${OUTDIR}/rootfs/home/finder.sh

sed -i 's|\.\./conf|conf|g' ${OUTDIR}/rootfs/home/finder-test.sh

# Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs
# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -o | gzip > ${OUTDIR}/initramfs.cpio.gz

echo "! Successfully Build Image and Rootfs !"
