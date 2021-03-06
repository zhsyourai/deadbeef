#!/usr/bin/env perl

# Find all C files, extract all configdlg and plugin_action properties/titles
# into localization files

use strict;
use warnings;
use FindBin qw'$Bin';
use lib "$FindBin::Bin/../perl_lib";
use lib "$FindBin::Bin";
use File::Find::Rule;
use Getopt::Long qw(GetOptions);
use HTML::Entities;
use extract;

my $help;
my $android_xml;
my $c_source;
my $out_fname;

GetOptions (
    "help|?" => \$help,
    "--android-xml" => \$android_xml,
    "--c-source" => \$c_source,
	"--output=s" => \$out_fname
) or die("Error in command line arguments\n");

if ($help) {
    print "Usage: $0 [options]\n";
    print "With no options, $0 will generate strings.pot from deadbeef source code\n\n";
    print "Options:\n";
    print "  --help               Show this text\n";
    print "  --android-xml        Generate android xml strings.xml\n";
    print "  --c-source           Generate strings.c file compatible with xgettext\n";
    exit (0);
}

my @lines = extract ($FindBin::Bin.'/../..', $android_xml);

my @unique_ids;

# the idea of the algorithm is to make it super quick to implement in C,
# with in-place generation support (given the string is ASCII).
sub string_to_id {
	my $s = shift;

	$s =~ s/[^a-zA-Z0-9_]/_/g;
	if ($s =~ /^([^a-zA-Z_])/) {
		$s = 'num_'.$s;
	}

	my $s_unique = $s;
	my $cnt = 1;
	if (grep ({ $_ eq $s_unique} @unique_ids)) {
		die "The generated ID is not unique: $s\n";
	}
	# NOTE: we want an algorithm which is very simple and fast,
	# and doesn't generate non-unique ids from unique strings.
	#while (grep ({ $_ eq $s_unique} @unique_ids)) {
	#	$s_unique = $s.$cnt;
	#	$cnt++;
	#}
	push @unique_ids, $s_unique;
	return $s_unique;
}


if ($android_xml) {
	my $fname = $out_fname // 'strings.xml';
    open XML, '>:encoding(utf8)', $fname or die "Failed to open $fname\n";
    print XML "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    print XML "<!-- This file is generated by $0, and contains localizable strings from all plugin configuration dialogs and actions -->\n";
    print XML "<resources>\n";
    for my $l (@lines) {
		my $id = string_to_id ($l->{msgid});
		my $value = encode_entities ($l->{msgid}, '\200-\377');
		$value =~ s/'/\\'/g;
        print XML "    <string name=\"$id\">$value</string>\n";
    }
    print XML "</resources>\n";
    close XML;
}
elsif ($c_source) {
	my $fname = $out_fname // 'strings.c';
    open C, '>:encoding(utf8)', $fname or die "Failed to open $fname\n";
    for my $l (@lines) {
        print C "_(\"$l->{msgid}\");\n";
    }
    close C;
}
else {
	my $fname = $out_fname // 'strings.pot';
    open POT, '>:encoding(utf8)', $fname or die "Failed to open $fname\n";

    print POT "msgid \"\"\nmsgstr \"\"\n\"Content-Type: text/plain; charset=UTF-8\\n\"\n\"Content-Transfer-Encoding: 8bit\\n\"\n";

    for my $l (@lines) {
        print POT "\n#: $l->{f}:$l->{line}\nmsgid \"$l->{msgid}\"\nmsgstr \"\"\n";
    }
    close POT;
}
