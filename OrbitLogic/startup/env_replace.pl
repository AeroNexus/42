#!/usr/bin/env perl

use strict;
use warnings;

# Use perl magic to automatically create backup file
#$^I = '.template';

# Replace any instance of $VAR with env value and print to file
# NOTE that there is a risk of doubly substituting for vars that start with the
# same characters, e.g. $APS_SLOT vs. $APS_SLOT_DIR; hence we use reverse sort
while (<>) {
    foreach my $key(reverse sort keys %ENV) {
        s/\$$key/$ENV{$key}/g
    }
    print;
}
