# cmusfm

[Last.fm](http://www.last.fm/) standalone scrobbler for the [cmus](https://cmus.github.io/) music player.

[![Build Status](https://github.com/Arkq/cmusfm/actions/workflows/build-and-test.yaml/badge.svg)](https://github.com/Arkq/cmusfm/actions/workflows/build-and-test.yaml)

## Features

* Listening now notification support
* Off-line played track cache for later submission
* POSIX ERE-based file name parser
* Desktop notification support (optionally)
* Customizable scrobbling service
* Small memory footprint

## Overview

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

By default cmusfm scrobbles to the Last.fm service. However, it is possible to change this
behavior by modifying `service-api-url` and `service-auth-url` options in the configuration file.
Afterwards, one should reinitialize cmusfm (`cmusfm init`) in order to authenticate with new
scrobbling service. In order to use [Libre.fm](https://libre.fm/) as a scrobbling service, one
shall use configuration as follows:

* `service-api-url = "https://libre.fm/2.0/"`
* `service-auth-url = "https://libre.fm/api/auth"`


## Installation

### Dependencies

* [cmus](https://cmus.github.io/)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libnotify](https://developer.gnome.org/libnotify/) (optional)

### Building and install

```shell
autoreconf --install
mkdir build && cd build
../configure --enable-libnotify
make && make install
```
### Install build dependencies

#### Debian/Ubuntu

```shell
sudo apt-get install build-essential pkg-config git
```
#### Making a .deb package

Building a Debian package directly from this repository is easy. Make sure you have the build dependencies installed as described above. Be sure it all builds, then do this:

```shell
sudo apt-get install fakeroot debhelper dpkg-dev
fakeroot dpkg-buildpackage -b -us -uc
```

You should then have a .deb package for you to install with dpkg -i.

#### Making a Debian source package for a PPA

The following commands require `git-buildpackage`:

```shell
git archive --format=tar HEAD | tar x && git commit -am "packaging: Makefile substitution"
gbp dch --commit --since=HEAD --upstream-branch=master --dch-opt="-lppa" --dch-opt="-D$(lsb_release -c -s)" debian
gbp buildpackage --git-upstream-tree=SLOPPY --git-export-dir=../build-area -S
```

You can then dput the *.changes file in `../build-area` to your PPA.

## Configuration

Before usage with the cmus music player, one has to grant access for the cmusfm in the Last.fm
service. To do so, simply run cmusfm with the `init` argument and follow the instruction. This
action might be also required when upgrading to the newer version with new features.

```shell
cmusfm init
```

After that you can safely edit `~/.config/cmus/cmusfm.conf` configuration file.

~~Note, that for some changes to take place, restart of the cmusfm server process is required. To
achieved this, one has to quit cmus player and then kill background instance of cmusfm (e.g. `pkill
cmusfm`).~~ Above statement is not valid if one has [inotify](http://en.wikipedia.org/wiki/Inotify)
subsystem available.

As a final step (after the access is granted in the Last.fm service) one should set cmusfm as a
status display program for cmus. This can be done by starting cmus and typing in the main window:

```shell
:set status_display_program=cmusfm
```

Enjoy!
