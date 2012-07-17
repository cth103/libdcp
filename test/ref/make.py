#!/usr/bin/python

import os
import sys
import fileinput
from lxml import etree

def replace(l, a, b):
    return l.replace(a, b)

assetmap_namespace = 'http://www.smpte-ra.org/schemas/429-9/2007/AM'
cpl_namespace = 'http://www.smpte-ra.org/schemas/429-7/2006/CPL'

wanted_cpl_id = 'df0e4141-13c3-4a7a-bef8-b5a04fcbc4bb'
wanted_pkl_id = '8e293965-f8ad-48c6-971d-261b01f65cdb'
wanted_assetmap_id = '18be072e-5a0f-44e1-b2eb-c8a52ae12789'
wanted_video_mxf_id = '81fb54df-e1bf-4647-8788-ea7ba154375b'
wanted_audio_mxf_id = 'c38bdd62-ce03-4988-8603-195f134207c7'

os.system('rm -rf DCP')
os.mkdir('DCP')
os.system('opendcp_mxf -i j2c -o DCP/video.mxf -r 24')
os.system('opendcp_mxf -i wav -o DCP/audio.mxf -r 24')
os.system('opendcp_xml --reel DCP/video.mxf DCP/audio.mxf -k feature -t "A Test DCP" -a "A Test DCP"')
os.system('mv *.xml DCP')

cpl_id = None
pkl_id = None
assetmap_id = None

for r, d, f in os.walk('DCP'):
    for n in f:
        if n.endswith('cpl.xml'):
            cpl_id = n[0:-8]
        elif n.endswith('pkl.xml'):
            pkl_id = n[0:-8]

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

for r, d, f in os.walk('DCP'):
    for n in f:
        if n.endswith('.xml'):
            for line in fileinput.input(os.path.join(r, n), inplace = 1):
                line = replace(line, cpl_id, wanted_cpl_id)
                line = replace(line, pkl_id, wanted_pkl_id)
                line = replace(line, assetmap_id, wanted_assetmap_id)
                line = replace(line, video_mxf_id, wanted_video_mxf_id)
                line = replace(line, audio_mxf_id, wanted_audio_mxf_id)
                print line,
                

