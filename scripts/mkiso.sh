#!/bin/bash
#
#

mkdir -p mnt
end=2047

init_img() {
  local name=$1
  local size=$2

  dd if=/dev/zero of=$name.img bs=512 count=$size
}

part() {
  parted -s disk.img "$@"
}

mk.vfat() {
  local part=$1
  local size=$(du -sB1024 $part | cut -f1)

  mkfs.vfat -C $part.img $size
  mtools -si $part.img $part/* ::
}

mk.ext() {
  local part=$1
  local size=$(du -sB4096 $part | cut -f1)

  mke2fs -d $part $part.img ${size}
}

mkpart() {
  local part=$1
  local fstype=$2
  local tbfs=$3

  size=$(expr 72 + $(du -sB512 $part | cut -f1))
  init_img $part $size
  mkfs.$fstype $part.img
  sudo mount -o loop $part.img mnt
  sudo cp -r $part/* mnt/
  sudo umount mnt

  local _end=$(expr $size + $end)
  cat $part.img >> disk.img
  part mkpart primary $tbfs $(expr $end + 1)s ${_end}s
  end=$_end
}

init_img disk 2048
part mklabel msdos

mkpart bootpart vfat fat16
mkpart datapart ext2 ext2
