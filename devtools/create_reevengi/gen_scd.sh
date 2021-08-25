#!/bin/bash

./xml2scd.pl re1/scd1.xml --defines > ../../engines/reevengi/re1/rdt_scd_defs.gen.h
./xml2scd.pl re1/scd1.xml --length > ../../engines/reevengi/re1/rdt_scd_lengths.gen.c
./xml2scd.pl re1/scd1.xml --types > ../../engines/reevengi/re1/rdt_scd_types.gen.h

./xml2scd.pl re2/scd2.xml --defines > ../../engines/reevengi/re2/rdt_scd_defs.gen.h
./xml2scd.pl re2/scd2.xml --length > ../../engines/reevengi/re2/rdt_scd_lengths.gen.c
./xml2scd.pl re2/scd2.xml --types > ../../engines/reevengi/re2/rdt_scd_types.gen.h

./xml2scd.pl re3/scd3.xml --defines > ../../engines/reevengi/re3/rdt_scd_defs.gen.h
./xml2scd.pl re3/scd3.xml --length > ../../engines/reevengi/re3/rdt_scd_lengths.gen.c
./xml2scd.pl re3/scd3.xml --types > ../../engines/reevengi/re3/rdt_scd_types.gen.h
