#!/bin/sh

LD_LIBRARY_PATH=build/src
build/test/tests
diff -ur build/test/foo test/ref/DCP
 