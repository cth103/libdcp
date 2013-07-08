#!/bin/bash

CINEMASLIDES=/home/carl/src/digital_cinema_tools/cinemaslides
CPL=/home/carl/src/libdcp-test/TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV/cpl_eece17de-77e8-4a55-9347-b6bab5724b9f_.xml

ls $CPL
export CINEMACERTSTORE=chain
$CINEMASLIDES -v debug --kdm --cpl $CPL --start 2013-07-06T20:04:58+00:00 --end 2023-07-02T20:04:56+00:00 --target target.pem --keysdir content_keys --formulation t1

