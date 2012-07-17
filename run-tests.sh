#!/bin/sh

LD_LIBRARY_PATH=build/src:build/asdcplib/src
build/test/tests
diff -ur build/test/foo test/ref/DCP
if [ "$?" != "0" ]; then
  echo "FAIL: files differ"
  exit 1
fi
echo "PASS"