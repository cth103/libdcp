#!/usr/bin/python

import os
import sys
import fileinput
from lxml import etree

assetmap_namespace = 'http://www.smpte-ra.org/schemas/429-9/2007/AM'
cpl_namespace = 'http://www.smpte-ra.org/schemas/429-7/2006/CPL'
pkl_namespace = 'http://www.smpte-ra.org/schemas/429-8/2007/PKL'

wanted_cpl_id = 'df0e4141-13c3-4a7a-bef8-b5a04fcbc4bb'
wanted_pkl_id = '8e293965-f8ad-48c6-971d-261b01f65cdb'
wanted_assetmap_id = '18be072e-5a0f-44e1-b2eb-c8a52ae12789'
wanted_video_mxf_id = '81fb54df-e1bf-4647-8788-ea7ba154375b'
wanted_audio_mxf_id = 'c38bdd62-ce03-4988-8603-195f134207c7'
wanted_reel_id = 'b135d5cf-d180-43d8-b0b3-7373737b73bf'
wanted_asset_hashes = ['E2vhyxdJQhEzSQZdp31w84ZZpfk=', '9OVODrw+zTkSbkGduoQ30k3Kk6Y=', '5E8Q9swcc2bBbFF3IEPNXfIP8gM=']
wanted_issue_date = '2012-07-17T04:45:18+00:00'

os.system('rm -rf DCP')
os.mkdir('DCP')
os.system('opendcp_mxf -i j2c -o DCP/video.mxf -r 24')
os.system('opendcp_mxf -i wav -o DCP/audio.mxf -r 24')
os.system('opendcp_xml --reel DCP/video.mxf DCP/audio.mxf -k feature -t "A Test DCP" -a "A Test DCP"')
os.system('mv *.xml DCP')

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
print asset_list
for a in asset_list.iter():
    if a.tag == "{%s}Hash" % pkl_namespace:
        asset_hashes.append(a.text)

issue_date =    xml.getroot().find(pkl_name('IssueDate')).text

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
                

