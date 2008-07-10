#!/bin/sh -x

libtoolize -c ; aclocal ; autoconf ; autoheader ; automake -a -c
