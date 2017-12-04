#!/bin/sh

set -e

LICENSE="$(dirname $0)/COPYING"
OUTFILE="$(dirname $0)/src/license.h"

echo "$(basename $0): generating ${OUTFILE} from ${LICENSE}"

echo '#ifndef LICENSE_H' >${OUTFILE}
echo '#define LICENSE_H 1' >>${OUTFILE}
echo "\n" >>${OUTFILE}
echo '#define ALL_LICENSES  \' >>${OUTFILE}
# first sed will escape all existing double quotes
# second sed will C-ify the text
cat "${LICENSE}" | sed 's/\"/\\"/g' | sed 's/\(.*\)/\"\1\\n\" \\/g' >>${OUTFILE}
echo '""' >>${OUTFILE}
echo "\n" >>${OUTFILE}
echo '#endif'  >>${OUTFILE}
