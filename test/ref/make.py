#!/usr/bin/python

#
# This slightly ridiculous script gets OpenDCP to generate
# a DCP using out test reference data (in j2c/ and wav/)
# and then adjusts its XML output to account for the fact
# that OpenDCP will generate its own random UUIDs (and use
# current timestamps).  We set UUIDs and timestamps back
# to what our test suite will expect.
#
# The output of this script is checked into git, so
# there's normally no need to run it.
#
# If you do run it, the XML should be right but the 
# MXFs that OpenDCP generates will not be quite what
# we expect, as they also contain random UUIDs.  I don't
# think there's an easy way round that, so after running
# this script you will need to check that the libdcp
# test code generates correct MXFs (by verification on
# a projector, probably), and then copy those MXFs into the
# test/ref/DCP directory.
#

import os
import sys
import fileinput
from lxml import etree

# Namespaces for the various XML files
assetmap_namespace = 'http://www.smpte-ra.org/schemas/429-9/2007/AM'
cpl_namespace = 'http://www.smpte-ra.org/schemas/429-7/2006/CPL'
pkl_namespace = 'http://www.smpte-ra.org/schemas/429-8/2007/PKL'

# The UUIDs of things that we want to put into the
# OpenDCP-generated XML
wanted_cpl_id = '9892e944-5046-4dbb-af7c-f50742f62fc2'
wanted_pkl_id = 'df0e4141-13c3-4a7a-bef8-b5a04fcbc4bb'
wanted_assetmap_id = 'b135d5cf-d180-43d8-b0b3-7373737b73bf'
wanted_video_mxf_id = '81fb54df-e1bf-4647-8788-ea7ba154375b'
wanted_audio_mxf_id = '67b9341e-cadd-4dac-9d5c-f5a1d59f2d06'
wanted_reel_id = '379fa64c-ad71-46cf-bef7-b45624006610'

# The hashes of the assets: first is the video MXF, second the audio MXF and third the CPL.
wanted_asset_hashes = ['VB9LCTmiD9OLlw4SvrEWUm5d67Q=', 'HapNpn7MjiJLa1OHRI61Rx8N/is=', 'PbXuvpUOKccTLMxg/lEbaXvNCT4=']

# The issue date that we want to use
wanted_issue_date = '2012-07-17T04:45:18+00:00'

# Get OpenDCP to make the DCP
os.system('rm -rf DCP')
os.mkdir('DCP')
os.system('opendcp_mxf -i j2c -o DCP/video.mxf -r 24')
os.system('opendcp_mxf -i wav -o DCP/audio.mxf -r 24')
os.system('opendcp_xml --reel DCP/video.mxf DCP/audio.mxf -k feature -t "A Test DCP" -a "A Test DCP"')
os.system('mv *.xml DCP')

# Find what IDs it used
cpl_id = None
pkl_id = None
assetmap_id = None
video_mxf_id = None
audio_mxf_id = None
reel_id = None

for r, d, f in os.walk('DCP'):
    for n in f:
        if n.endswith('cpl.xml'):
            cpl_id = n[0:-8]
        elif n.endswith('pkl.xml'):
            pkl_id = n[0:-8]

# (along the way, rename the CPL/PKL files)
os.rename('DCP/%s_cpl.xml' % cpl_id, 'DCP/%s_cpl.xml' % wanted_cpl_id)
os.rename('DCP/%s_pkl.xml' % pkl_id, 'DCP/%s_pkl.xml' % wanted_pkl_id)

xml = etree.parse('DCP/ASSETMAP.xml')
assetmap_id = xml.getroot().find('{%s}Id' % assetmap_namespace).text
assetmap_id = assetmap_id.replace('urn:uuid:', '')

def cpl_name(s):
    return '{%s}%s' % (cpl_namespace, s)

xml = etree.parse('DCP/%s_cpl.xml' % wanted_cpl_id)

video_mxf_id = xml.getroot().find(cpl_name('ReelList')).    \
                             find(cpl_name('Reel')).        \
                             find(cpl_name('AssetList')).   \
                             find(cpl_name('MainPicture')). \
                             find(cpl_name('Id')).text
video_mxf_id = video_mxf_id.replace('urn:uuid:', '')

audio_mxf_id = xml.getroot().find(cpl_name('ReelList')).    \
                             find(cpl_name('Reel')).        \
                             find(cpl_name('AssetList')).   \
                             find(cpl_name('MainSound')). \
                             find(cpl_name('Id')).text
audio_mxf_id = audio_mxf_id.replace('urn:uuid:', '')

reel_id =      xml.getroot().find(cpl_name('ReelList')).    \
                             find(cpl_name('Reel')).        \
                             find(cpl_name('Id')).text
reel_id = reel_id.replace('urn:uuid:', '')

def pkl_name(s):
    return '{%s}%s' % (pkl_namespace, s)

xml = etree.parse('DCP/%s_pkl.xml' % wanted_pkl_id)

asset_list =   xml.getroot().find(pkl_name('AssetList'))
asset_hashes = []
for a in asset_list.iter():
    if a.tag == "{%s}Hash" % pkl_namespace:
        asset_hashes.append(a.text)

issue_date =    xml.getroot().find(pkl_name('IssueDate')).text

# Now run through the XML files doing the replacements
for r, d, f in os.walk('DCP'):
    for n in f:
        if n.endswith('.xml'):
            for line in fileinput.input(os.path.join(r, n), inplace = 1):
                line = line.replace(cpl_id, wanted_cpl_id)
                line = line.replace(pkl_id, wanted_pkl_id)
                line = line.replace(assetmap_id, wanted_assetmap_id)
                line = line.replace(video_mxf_id, wanted_video_mxf_id)
                line = line.replace(audio_mxf_id, wanted_audio_mxf_id)
                line = line.replace(reel_id, wanted_reel_id)
                line = line.replace(issue_date, wanted_issue_date)
                for i in range(0, len(asset_hashes)):
                    line = line.replace(asset_hashes[i], wanted_asset_hashes[i])
                print line,
                

