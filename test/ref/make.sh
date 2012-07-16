#!/bin/sh

rm -rf DCP
mkdir DCP
opendcp_mxf -i j2c -o DCP/video.mxf -r 24
opendcp_mxf -i wav -o DCP/audio.mxf -r 24
opendcp_xml --reel DCP/video.mxf DCP/audio.mxf -k feature -t "A Test DCP" -a "A Test DCP"
mv *.xml DCP
mv DCP/*_pkl.xml DCP/04040404-0404-0404-0404-040404040404_pkl.xml
mv DCP/*_cpl.xml DCP/02020202-0202-0202-0202-020202020202_cpl.xml
