#!/bin/sh -x

# Detect OCaml version and symlink in right runtime files
OCAML_VERSION=`ocamlc -version`
case $OCAML_VERSION in
4.00.1|4.01.0)
  ln -nsf ocaml.$OCAML_VERSION runtime/ocaml
  ln -nsf caml.$OCAML_VERSION runtime/include/caml
  ;;
*)
  echo Unknown OCaml version $OCAML_VERSION
  exit 1
esac

# This extra flag only needed for gcc 4.8+
GCC_MVER2=`gcc -dumpversion | cut -f2 -d.`
if [ $GCC_MVER2 -ge 8 ]; then
  EXTRA_CFLAGS=-fno-tree-loop-distribute-patterns
fi

TARGET=TARGET_amd64
TARGET_ARCH_FAM=arm

case "$1" in
xen)
  CC=${CC:-cc}
  PWD=`pwd`
  GCC_INCLUDE=`env LANG=C ${CC} -print-search-dirs | sed -n -e 's/install: \(.*\)/\1/p'`
  CFLAGS="$EXTRA_CFLAGS -O3 -U __linux__ -U __FreeBSD__ -U __sun__ -D__MiniOS__ -D__MiniOS__ \
    -D__XEN_INTERFACE_VERSION__=0x00030205 -D__INSIDE_MINIOS__ -nostdinc -std=gnu99 \
    -fno-reorder-blocks -fstrict-aliasing \
    -I${GCC_INCLUDE}/include \
    -isystem ${PWD}/runtime/include/ -isystem ${PWD}/runtime/include/mini-os \
    -isystem ${PWD}/runtime/include/mini-os/${TARGET_ARCH_FAM} -DCAML_NAME_SPACE -D${TARGET}  \
    -DSYS_xen -I${PWD}/runtime/ocaml -I${PWD}/runtime/libm \
    -Wextra -Wchar-subscripts -Wno-switch \
    -Wno-unused -Wredundant-decls -D__dietlibc__ -I${PWD}/runtime/dietlibc \
    -DNATIVE_CODE"
  ;;
*)
  CC="${CC:-cc}"
  CFLAGS="-Wall -O3"
  ;;
esac
