#!/usr/bin/perl
use strict;
use warnings;

use XML::LibXML;

sub GenerateDefines {
	my ($xml) = @_;

	my $e= $xml->findnodes( "/scd/*");
	foreach my $e (@$e) {
		if ($e->nodeName ne "inst") {
			next;
		}

		my $inst_name_up = uc $e->getAttribute('name');
		my $inst_id = $e->getAttribute('id');

		printf("#define INST_%s\t%s\n", $inst_name_up, $inst_id);
	}
}

sub GenerateTypeFields {
	my ($node) = @_

	foreach my $child ( $node->getChildnodes ) {
        if ( $child->nodeType() != XML_ELEMENT_NODE ) {
			next;
        }

        print "\t", $child->nodeName(), ":", $child->textContent(), "\n";
    }
/*
	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "field")!=0) {
			continue;
		}

		field_name = xmlGetProp(child, "name");
		field_type = xmlGetProp(child, "type");
		field_array = xmlGetProp(child, "array");

		if (strcmp(field_type, "union")==0) {
			printf("\t%s_u\t%s;\n", field_name, field_name);
		} else {
			if (field_array) {
				printf("\t%s\t%s[%s];\n", field_type, field_name, field_array);
			} else {
				printf("\t%s\t%s;\n", field_type, field_name);
			}
		}
	}
*/
}

sub GenerateTypes {
	my ($xml) = @_;

	my $nodes= $xml->findnodes( "/scd/*");
	foreach my $e (@$nodes) {
		if ($e->nodeName ne "inst") {
			next;
		}

		my $inst_name_low= lc $e->getAttribute('name');

		printf(	"\n".
			"typedef struct {\n");
		GenerateTypeFields($e);
		printf("} script_inst_%s_t;\n", $inst_name_low);
	}

	# Union containing all instructions
	printf(	"\n".
		"typedef union script_inst_u script_inst_t;\n".
		"\n".
		"union script_inst_u {\n".
		"\tuint8\topcode;\n");

	foreach my $e (@$nodes) {
		if ($e->nodeName ne "inst") {
			next;
		}

		my $inst_name_low= lc $e->getAttribute('name');

		printf("\tscript_inst_%s_t\ti_%s;\n", $inst_name_low, $inst_name_low);
	}

	printf("};\n");
}

# Read parameters
if (scalar(@ARGV)<2) {
	printf STDERR "Parameters: xml2scl.pl /path/to/scdN.xml [--defines|--types|--enums|--dumps|--lengths|--rewiki]\n";
	exit(1);
}

my $dom = XML::LibXML->load_xml(
    location => $ARGV[0]
);
#my $e= $dom->findnodes( "/scd/inst/*");
#foreach my $e (@$e) {
#	print $e->nodeName;
#}

if ($ARGV[1] eq "--defines") {
	GenerateDefines( $dom );
}
if ($ARGV[1] eq "--types") {
	GenerateTypes( $dom );
}
if ($ARGV[1] eq "--enums") {
	print "enums\n";
}
if ($ARGV[1] eq "--dumps") {
	print "dumps\n";
}
if ($ARGV[1] eq "--length") {
	print "length\n";
}
if ($ARGV[1] eq "--rewiki") {
	print "rewiki\n";
}
exit(0);
