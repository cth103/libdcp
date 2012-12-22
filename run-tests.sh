#!/bin/bash

#
# Runs our test suite, which builds a DCP.
# The output is compared against the one
# in test/ref/DCP, and an error is given
# if anything is different.
#

if [ "$1" == "--debug" ]; then
  shift
  LD_LIBRARY_PATH=build/src:build/asdcplib/src gdb --args build/test/tests
else
  LD_LIBRARY_PATH=build/src:build/asdcplib/src build/test/tests
fi
diff -ur test/ref/DCP build/test/foo
if [ "$?" != "0" ]; then
  echo "FAIL: files differ"
  exit 1
fi

rm -f build/test/info.log

if [ ! -e "../libdcp-test" ]; then
  echo "Test corpus not found"
  exit 1
fi

for d in `find ../libdcp-test -mindepth 1 -maxdepth 1 -type d`; do
  if [ `basename $d` != ".git" ]; then
    LD_LIBRARY_PATH=build/src:build/asdcplib/src build/tools/dcpinfo -s $d >> build/test/info.log
    if [ "$?" != "0" ]; then
      echo "FAIL: dcpinfo failed for $d"
      exit 1
    fi
  fi
done

diff -q build/test/info.log ../libdcp-test/info.log
if [ "$?" != "0" ]; then
  echo "FAIL: dcpinfo output incorrect"
  exit 1
fi

rm -f build/test/info2.log
rm -rf build/test/libdcp-test

cp -r ../libdcp-test build/test
for d in `find build/test/libdcp-test -mindepth 1 -maxdepth 1 -type d`; do
  if [ `basename $d` != ".git" ]; then
    LD_LIBRARY_PATH=build/src:build/asdcplib/src build/test/rewrite_subs $d
    LD_LIBRARY_PATH=build/src:build/asdcplib/src build/tools/dcpinfo -s $d >> build/test/info2.log
  fi
done

diff -q build/test/info2.log ../libdcp-test/info.log
if [ "$?" != "0" ]; then
  echo "FAIL: dcpinfo output from rewrite incorrect"
  exit 1
fi
    
echo "PASS"
