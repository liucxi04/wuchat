#!/bin/sh

if [ ! -d bin/module ]
then
    mkdir bin/module
else
    unlink bin/chat
    unlink bin/module/libchat.so
fi

cp luwu/bin/luwu bin/chat
cp lib/libchat.so bin/module/
