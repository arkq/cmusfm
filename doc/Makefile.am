# BlueALSA - Makefile.am
# Copyright (c) 2016-2021 Arkadiusz Bokowy

man1_MANS = cmusfm.1

SUFFIXES = .rst
MOSTLYCLEANFILES = $(man1_MANS)

.rst:
	$(SED) 's/\$$VERSION\$$/@PACKAGE_STRING@/' $< | $(RST2MAN) - $@
