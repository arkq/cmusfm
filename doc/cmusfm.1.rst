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

**cmusfm** *COMMAND*

DESCRIPTION
===========

**cmusfm** is a standalone Last.fm scrobbler for ``cmus(1)`` music player.

COMMANDS
========

init
    Initialize **cmusfm**.

    This command will try to grant access to your Last.fm account. On a
    subsequent run it will check whether access to Last.fm account is still
    valid. In case of failure, it will prompt you to grant access. Running
    this command might also be necessary to reinitialize configuration
    after **cmusfm** upgrade.

    After successful initialization, **cmusfm** will store your Last.fm
    credentials in the ``~/.config/cmus/cmusfm.conf`` configuration file.
    See the FILES_ section for details.

    As a final step it is necessary to set **cmusfm** as a status display
    program for ``cmus(1)``. This can be done by starting **cmus** and typing
    in the main window:

    ``:set status_display_program=cmusfm``

server
    Manually run **cmusfm** server.

    This command is useful when you want to debug **cmusfm**. During normal
    operation **cmusfm** server is started automatically, so you don't need
    to use this command.

FILES
=====

~/.config/cmus/cmusfm.conf
    **cmusfm** configuration file.

    Available configuration options:

    * **format-localfile** - regular expression used for parsing local file
      names (default: ``"^(?A.+) - (?T.+)\.[^.]+$"``)
    * **format-shoutcast** - regular expression used for parsing shoutcast
      stream names (default: ``"^(?A.+) - (?T.+)$"``)
    * **format-coverfile** - regular expression used for matching cover file
      names (default: ``"^folder\.jpg$"``)

    * **now-playing-localfile** - report now-playing status for local files
      (default: ``"yes"``)
    * **now-playing-shoutcast** - report now-playing status for shoutcast
      streams (default: ``"yes"``)
    * **submit-localfile** - scrobble local files (default: ``"yes"``)
    * **submit-shoutcast** - scrobble shoutcast streams (default: ``"yes"``)
    * **notification** - show desktop notification when playing a track
      (default: ``"no"``)

    * **service-api-url** - URL of the Last.fm API service (default:
      ``"https://ws.audioscrobbler.com/2.0/"``); after changing this
      option you might need to reinitialize **cmusfm**.
    * **service-auth-url** - URL of the Last.fm authentication service
      (default: ``"https://www.last.fm/api/auth/"``); after changing this
      option you might need to reinitialize **cmusfm**.

    Available regexp matched subgroups:

    * **(?A...)** - match artist name
    * **(?B...)** - match album name
    * **(?N...)** - match track number
    * **(?T...)** - match track title

    All matched subgroups shall be used only once. Otherwise the first
    occurrence will be used. Note, that using matched subgroups without
    the extension notation (e.g.: ``(.+)``) might result in an unexpected
    behavior.

COPYRIGHT
=========

Copyright (c) 2014-2022 Arkadiusz Bokowy.

The cmusfm project is licensed and distributed under the terms of the GNU
General Public License.

SEE ALSO
========

``cmus(1)``, ``cmus-remote(1)``, ``cmus-tutorial(7)``

Project web site
  https://github.com/arkq/cmusfm
