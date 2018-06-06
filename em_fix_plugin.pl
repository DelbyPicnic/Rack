#!/usr/bin/perl

my $bc = 'plugin.bc';
my $ll = 'plugin.ll';
my $slug = $ARGV[0];
my %names;

$slug =~ s/[^a-z0-9]//ig;

open F, '<', $ll;

while (<F>) {
	if ($_ =~ /^\@([_A-Za-z0-9_]+) =/) {
		if (not $_ =~ /= (external|internal|private|comdat)/) {
			$n = quotemeta $1;
			$names{$1} = 1;
		}
	}
	elsif ($_ =~ /^define[a-z0-9 ]+\@([_A-Za-z0-9_]+)\(/) {
		if (not $_ =~ /(external|internal|private|comdat)/) {
			$n = quotemeta $1;
			$names{$1} = 1;
		}
	}
}

$r = '(\b)(' . join ('|', keys %names) . ')(\b)';

seek F, 0, 0;

while (<F>) {
	$_ =~ s/$r/\1\2_$slug\3/g;
	$_ =~ s/define void \@init\(/define void \@init_$slug\(/;
	print $_;
}

close F;
