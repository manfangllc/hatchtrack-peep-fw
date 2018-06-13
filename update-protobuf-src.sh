#! /bin/bash

# TODO: I really want to remove this script and find some way to get it into
# the build system. Currently, if updates are made to the protobuf source, this
# script will need to be manually run and then committed to git.

set -e
set -x

SRC_DIR=./lib/hatchtrack-peep-protobuf/src
OUT_DIR=./protobuf
NANOPB_DIR=./lib/nanopb

SRC=$SRC_DIR/*.proto

PROTOC_PLUGIN_C="--plugin=protoc-gen-nanopb=$NANOPB_DIR/generator/protoc-gen-nanopb"
PROTOC_OPTS="-I=$SRC_DIR --nanopb_out=-I$SRC_DIR:$OUT_DIR $PROTOC_PLUGIN_C"

make -C $NANOPB_DIR/generator/proto || exit

protoc $PROTOC_OPTS $SRC

cp $NANOPB_DIR/pb.h $OUT_DIR
cp $NANOPB_DIR/pb_encode.* $OUT_DIR
cp $NANOPB_DIR/pb_decode.* $OUT_DIR
cp $NANOPB_DIR/pb_common.* $OUT_DIR

# TODO: Might need to provide a patch in the future for PB_BYTES_ARRAY_T to
# account for its "size" parameter getting set to a uint8_t type. Something
# like the following will work...
#-typedef PB_BYTES_ARRAY_T(4096) Command_FileContents_contents_t;
#+//typedef PB_BYTES_ARRAY_T(4096) Command_FileContents_contents_t;
#+// MREUTMAN: need to manually patch since "size" was getting set to uint8_t
#+typedef struct {
#+       uint32_t size;
#+       uint8_t bytes[4096];
#+} Command_FileContents_contents_t;
#+

patch -p 1 < patch/command.pb.h.patch
