#! /usr/bin/perl

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

# This file is a part of Binc IMAP.

sub authenticated {
  my ($user,$pass) = @_;
  # This is the only function you need to change.

  # If password is OK, change to the right home area
  # and return 1, otherwise return 0.
  # On error exit(111);
  exit(111);
}

use strict;

my ($in, $len, $buf);
open (FD3, "<&=3") or exit(111);
$len = read(FD3, $buf, 512);
close FD3;

exit(111) if $len < 4;

my($user, $pass) = split /\x00/, $buf;
$user = lc $user;

if (authenticated($user, $pass)) {
    exec @ARGV;
} else {
    exit 1;
}
