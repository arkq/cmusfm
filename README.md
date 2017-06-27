cmusfm [![Build Status](https://travis-ci.org/Arkq/cmusfm.svg?branch=master)](https://travis-ci.org/Arkq/cmusfm)
======

[Last.fm](http://www.last.fm/) standalone scrobbler for the [cmus](https://cmus.github.io/) music
player.

Features:

* Listening now notification support
* Off-line played track cache for later submission
* POSIX ERE-based file name parser
* Desktop notification support (optionally)
* Customizable scrobbling service
* Small memory footprint

When discography is correctly tagged - at least artist and title field - scrobbling needs no
further configuration (see: [Configuration](#configuration)). However, if this requirement is not
met, then one can use POSIX ERE-based file name parser feature. But what the heck is this?

Cmusfm allows to configure regular expression patterns for local files and for shoutcast streaming
services. The syntax is compatible with the [POSIX Extended Regular
Expression](http://en.wikipedia.org/wiki/Regular_expression#Standards) (ERE) with one exception.
Matched subexpression has to be marked with the `?` extension notation. There are four distinguish
types:

* `(?A...)` - match artist name
* `(?B...)` - match album name
* `(?N...)` - match track number
* `(?T...)` - match track title

All extension types can be used only once, otherwise only the first occurrence will be used. Also
note, that using matched subexpressions without the extension notation might result in an
unexpected behavior. Default configuration assumes formats as follows:

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
an extra feature, it is disabled by default in the cmusfm configuration file too. Note, that cover
art file has to be explicitly stored in the current track's directory - embedded covers are not
displayed. Exemplary configuration might be as follows:

* `notification = "yes"`
* `format-coverfile = "^(cover|folder)\.jpg$"`


Installation
------------

	$ autoreconf --install
	$ mkdir build && cd build
	$ ../configure --enable-libnotify
	$ make && make install


Configuration
-------------

Before usage with the cmus music player, one has to grant access for the cmusfm in the Last.fm
service. To do so, simply run cmusfm with the `init` argument and follow the instruction. This
action might be also required when upgrading to the newer version with new features.

	$ cmusfm init

After that you can safely edit `~/.config/cmus/cmusfm.conf` configuration file.

By default cmusfm scrobbles to the Last.fm service. However, it is possible to change this
behavior by modifying `service-api-url` and `service-auth-url` options in the configuration file.
Afterwards, one should reinitialize cmusfm (`cmusfm init`) in order to authenticate with new
scrobbling service. In order to use [Libre.fm](https://libre.fm/) as a scrobbling service, one
shall use configuration as follows:

* `service-api-url = "https://libre.fm/2.0/"`
* `service-auth-url = "https://libre.fm/api/auth"`

~~Note, that for some changes to take place, restart of the cmusfm server process is required. To
achieved this, one has to quit cmus player and then kill background instance of cmusfm (e.g.
`pkill cmusfm`).~~ Above statement is not valid if one's got
[inotify](http://en.wikipedia.org/wiki/Inotify) subsystem available.

As a final step (after the access is granted in the Last.fm service) one should set cmusfm as a
status display program for cmus. This can be done by starting cmus and typing in the main window:

	:set status_display_program=cmusfm

Enjoy!
