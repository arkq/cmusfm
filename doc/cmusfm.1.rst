======
cmusfm
======

---------------------------------------
Last.fm scrobbler for cmus music player
---------------------------------------

:Date: June 2022
:Manual section: 1
:Manual group: General Commands Manual
:Version: $VERSION$

SYNOPSIS
========

**cmusfm** [*OPTION*]

OPTIONS
=======

int
    One has to grant access for the cmusfm in the Last.fm service. This
    actionmight be also required when upgrading to the newer version with new
    features.

server
    Show cmusfm string.

DESCRIPTION
===========

**cmusfm** is an Open Source tool Last.fm standalone scrobbler for the cmus
music player.

**Features**

* Listening now notification support
* Off-line played track cache for later submission
* POSIX ERE-based file name parser
* Desktop notification support (optionally)
* Customizable scrobbling service
* Small memory footprint

**Overview**

When discography is correctly tagged - at least artist and title field -
scrobbling needs no further configuration (see: CONFIGURATION_). However, if this
requirement is not met, then one can use POSIX ERE-based file name parser feature.
But what the heck is this?

Cmusfm allows to configure regular expression patterns for local files and for
shoutcast streaming services. The syntax is compatible with the POSIX Extended
Regular Expression (ERE) with one exception. Matched subexpression has to be
marked with the ? extension notation. There are four distinguish types:

* `(?A...)` - match artist name
* `(?B...)` - match album name
* `(?N...)` - match track number
* `(?T...)` - match track title

All extension types can be used only once, otherwise only the first occurrence
will be used. Also note, that using matched subexpressions without the extension
notation might result in an unexpected behavior. Default configuration assumes
formats as follows:

* `format-localfile = "^(?A.+) - (?T.+)\.[^.]+$"` (matches: The Beatles - Yellow Submarine.ogg)
* `format-shoutcast = "^(?A.+) - (?T.+)$"` (matches: The Beatles - Yellow Submarine)

Scrobbling behavior and now playing notification can be controlled via the
following self-explainable options (default is "yes" for all of them):

* `now-playing-localfile = "yes"`
* `now-playing-shoutcast = "no"`
* `submit-localfile = "yes"`
* `submit-shoutcast = "no"`

Cmusfm provides also one extra feature, which was mentioned earlier - desktop
notifications. In order to have this functionality, one has to enable it during
the compilation stage. Since it is an extra feature, it is disabled by default
in the cmusfm configuration file too. Note, that cover art file has to be
explicitly stored in the current track's directory - embedded covers are not
displayed. Exemplary configuration might be as follows:

* `notification = "yes"`
* `format-coverfile = "^(cover|folder)\.jpg$"`

By default cmusfm scrobbles to the Last.fm service. However, it is possible to
change this behavior by modifying service-api-url and service-auth-url options
in the configuration file. Afterwards, one should reinitialize cmusfm
(`cmusfm init`) in order to authenticate with new scrobbling service. In order
to use Libre.fm as a scrobbling service, one shall use configuration as follows:

* `service-api-url = "https://libre.fm/2.0/"`
* `service-auth-url = "https://libre.fm/api/auth"`


CONFIGURATION
=============
Before usage with the cmus music player, one has to grant access for the cmusfm
in the Last.fm service. To do so, simply run cmusfm with the init argument and
follow the instruction. This action might be also required when upgrading to the
newer version with new features.

`cmusfm init`

After that you can safely edit `~/.config/cmus/cmusfm.conf` configuration file.
As a final step (after the access is granted in the Last.fm service) one should
set cmusfm as a status display program for cmus. This can be done by starting
cmus and typing in the main window:

`:set status_display_program=cmusfm`

FILES
=====
`~/.config/cmus/cmusfm.conf` cmusfm configuration file.


SEE ALSO
========

``cmusfm(1)``, ``cmus-remote(1)``, ``cmus-tutorial(7)``

Project web site at https://github.com/Arkq/cmusfm

COPYRIGHT
=========

Copyright (c) 2014-2022 Arkadiusz Bokowy.

The cmusfm project is licensed under the terms of the distributed under the
GNU General Public License.
