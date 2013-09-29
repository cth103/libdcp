#!/bin/bash

CINEMASLIDES=/home/carl/src/digital_cinema_tools/cinemaslides
#CPL=/home/carl/src/libdcp-test/TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV/cpl_eece17de-77e8-4a55-9347-b6bab5724b9f_.xml
CPL=/home/carl/DCP/kdmtest/KDMTEST_TST-1_F_51_2K_20130928/fbb1d2ce-fcd9-4765-8f01-2afcad274506_cpl.xml

ls $CPL
export CINEMACERTSTORE=/home/carl/.config/dcpomatic/crypt
$CINEMASLIDES -v debug --kdm --cpl $CPL --start 2013-09-28T01:41:51+00:00 --end 2014-09-28T01:41:51+00:00 --target target.pem --keysdir content_keys

