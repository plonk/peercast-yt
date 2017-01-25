#!/usr/bin/perl

# Peercact URL handler for xmms.
# By Romain Beauxis <toots@rastageeks.org>

# Usage: peercast-play-in-xmms.pl <peercast://foo>
# This script search for a string in the following mask: peercast://pls/HASHNUMBER
# And then call xmms to enqueue the URL: http://127.0.0.1:7144/stream/HASHNUMBER
# WARNING: this does not handle any parametrs like ?foo after the HASH, 
# But xmms seems to accept it.

my $args = shift;

$args =~ /peercast:\/\/pls\/(\w*)/;

my $regex = $1;

exec("xmms --play \"http://127.0.0.1:7144/stream/$regex\"")
