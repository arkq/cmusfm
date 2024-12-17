# cmusfm - Makefile.am
# SPDX-FileCopyrightText: 2016-2024 Arkadiusz Bokowy and contributors
# SPDX-License-Identifier: GPL-3.0-or-later

man1_MANS = cmusfm.1

SUFFIXES = .rst
MOSTLYCLEANFILES = $(man1_MANS)

.rst:
	sed 's/\$$VERSION\$$/@PACKAGE_STRING@/' $< | $(RST2MAN) - $@
