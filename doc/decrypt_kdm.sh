#!/bin/bash

KDM_DECRYPT=/home/carl/src/digital_cinema_tools/encryption/kdm-decrypt.rb
KDM=kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml

$KDM_DECRYPT $KDM target_private.key
