#!/bin/sh
set -e -x

# construct root
# python3 ../../zdtm.py run -t zdtm/static/env00 --iter 0 -f ns

dd if=/dev/zero of=zdtm.detached.img bs=1M count=100
mkfs.ext4 zdtm.detached.img
dev=`losetup --find --show zdtm.detached.img`
export ZDTM_DETACHED_MNT=$dev
python3 ../../zdtm.py run $EXTRA_OPTS -t zdtm/static/detached_loop_mnt || ret=$?
rm zdtm.detached.img
exit $ret
