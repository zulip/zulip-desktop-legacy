#!/usr/bin/env python

# Updates a sparkle appcast xml file with a new release

from optparse import OptionParser
from xml.dom.minidom import parse
from datetime import datetime

import os

parser = OptionParser(r"""

%prog --f xmlfile -s "signature" -v X.X.X -l <bytelength>""")
parser.add_option("-f", "--file",      dest="filename",
	help="XML appcast file to update", metavar="FILE")
parser.add_option("-s", "--sig",       dest="sig",
	help="Signature from Sparkle signing process")
parser.add_option("-v", "--version",   dest="version",
	help="New version to update the appcast file with")
parser.add_option("-l", "--length",    dest="length",
	help="Length in bytes of .tar.bz2 file")
parser.add_option("-w", "--windows",   dest="windows",
	help="Write a windows sparkle formatted XML file",
	action="store_true", default=False)

(options, _) = parser.parse_args()

if not options.windows and (options.filename is None or
	options.sig is None or options.version is None or
	options.length is None):
	parser.error("Please pass all four required arguments")
elif options.windows and (options.filename is None or options.version is None):
	parser.error("Please provide an XML filename and version string")

try:
	xml = parse(options.filename)
except IOError:
	print "Failed to parse filename: %s" % options.filename
	os.exit(1)

pubDate = datetime.now().strftime("%a, %d %b %Y %H:%M:%S %z")
version = "Version %s" % options.version
if options.windows:
	path = "dist/apps/win/zulip-%s.exe" % (options.version,)
else:
	path = "dist/apps/mac/Zulip-%s.tar.bz2" % (options.version,)

url = "https://zulip.com/%s" % (path,)

channel = xml.getElementsByTagName('channel')[0]
latest = channel.getElementsByTagName('item')[0]

newItem = latest.cloneNode(True)
newItem.getElementsByTagName('title')[0].firstChild.replaceWholeText(version)
newItem.getElementsByTagName('pubDate')[0].firstChild.replaceWholeText(pubDate)
newItem.getElementsByTagName('enclosure')[0].setAttribute("url", url)
newItem.getElementsByTagName('enclosure')[0].setAttribute("sparkle:version",
														  options.version)

if not options.windows:
	newItem.getElementsByTagName('enclosure')[0].setAttribute("length",
															  options.length)
	newItem.getElementsByTagName('enclosure')[0].setAttribute("sparkle:dsaSignature",
														      options.sig)

channel.insertBefore(newItem, latest)

outfile = open(options.filename, 'w')
xml.writexml(outfile)
outfile.close()
