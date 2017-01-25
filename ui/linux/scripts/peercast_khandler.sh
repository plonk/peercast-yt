#!/bin/sh

# Peercast kmenu for peercast:// links
# By Romain Beauxis <toots@rastageeks.org>

# You can add your own services.
# Just have a look to those for xmms, you'll see how it works.
# Then add yours to the same directory, and add it to the list below.

SERVICES="enqueue-in-xmms.pl play-in-xmms.pl"

binary=$(kdialog --title "Peercast Handler" --combobox Service $SERVICES 2> /dev/null)

/usr/local/lib/peercast/scripts/peercast-$binary $1

