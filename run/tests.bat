set PATH=%PATH%;c:\users\ci\bin_v2.17.x;c:\users\ci\workspace\libdcp\bin;c:\users\ci\workspace\libdcp\lib
xcopy ..\libdcp\tags build\tags\
xcopy ..\libdcp\ratings build\
build\test\tests.exe --log_level=test_suite -- c:\users\ci\libdcp-test-private
