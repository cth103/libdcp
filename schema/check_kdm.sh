#!/bin/bash
xmllint --path . --nonet --noout --schema SMPTE-430-1-2006-Amd-1-2009-KDM.xsd $1
