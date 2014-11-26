cmusfm
======

[Last.fm](http://www.last.fm/) standalone scrobbler for the [cmus](https://cmus.github.io/) music
player.

Features:

* Listening now notification support
* Off-line played track cache for later submission
* POSIX ERE-based file name parser
* Desktop notification support (optionally)
* Small memory footprint

When discography is correctly tagged - at least artist and title field - scrobbling needs no
further configuration (see: Configuration). However if this requirement is not met, then one can
use POSIX ERE-based file name parser feature. But what the heck is this?

Cmusfm allows to configure regular expression patterns for local files and for shoutcast streaming
services. The syntax is compatible with the [POSIX Extended Regular
Expression](http://en.wikipedia.org/wiki/Regular_expression#Standards) (ERE) with one exception.
Matched subexpression has to be marked with the `?` extension notation. There are four distinguish
types:

* `(?A...)` - match artist name
* `(?B...)` - match album name
* `(?N...)` - match track number
* `(?T...)` - match track title

All extension types can be used only once, otherwise only the first occurrence will be used. Using
matched subexpressions without extension is not forbidden, however can result in an unexpected
behavior. Default configuration assumes formats as follows:

* `format-localfile = "^(?A.+) - (?T.+)\.[^.]+$"` (matches: The Beatles - Yellow Submarine.ogg)
* `format-shoutcast = "^(?A.+) - (?T.+)$"` (matches: The Beatles - Yellow Submarine)

Scrobbling behavior and now playing notification can be controlled via the following
self-explainable options (default is "yes" for all of them):

* `now-playing-localfile = "yes"`
* `now-playing-shoutcast = "no"`
* `submit-localfile = "yes"`
* `submit-shoutcast = "no"`

Cmusfm provides also one extra feature, which was mentioned earlier - desktop notifications. In
order to have this functionality, one has to enable it during the compilation stage. Since it is
extra, it is disabled by default in the cmusfm configuration file too. Note, that cover art file
has to be explicitly stored in the current track's directory - embedded covers are not displayed.
Exemplary configuration might be as follows:

* `notification = "yes"`
* `format-coverfile = "^(cover|folder)\.jpg$"`


Instalation
-----------

	$ autoreconf --install
	$ mkdir build && cd build
	$ ../configure --enable-libnotify
	$ make && make install


Configuration
-------------

Before usage with the cmus music player, one have to grant access for the cmusfm in the Last.fm
service. To to so, simply run cmusfm with the `init` argument and follow instructions. This action
can be also required when upgrading to newer version with new features available.

	$ cmusfm init

After that you can safely edit `~/.config/cmus/cmusfm.conf` configuration file.

~~Note, that for some changes to take place restart of the cmusfm server is required. To achieved
this, one has to quit cmus player and then kill the cmusfm background instance (e.g. `pkill
cmusfm`).~~ Above statement is not valid if one's got
[inotify](http://en.wikipedia.org/wiki/Inotify) subsystem available.
