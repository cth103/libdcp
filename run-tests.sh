#!/bin/sh

#
# Runs our test suite, which builds a DCP.
# The output is compared against the one
# in test/ref/DCP, and an error is given
# if anything is different.
#

LD_LIBRARY_PATH=build/src:build/asdcplib/src
build/test/tests
diff -ur build/test/foo test/ref/DCP
if [ "$?" != "0" ]; then
  echo "FAIL: files differ"
  exit 1
fi
echo "PASS"