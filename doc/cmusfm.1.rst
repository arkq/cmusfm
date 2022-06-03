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
    this command might also be necessary to reinitialize configuration in
    case of **cmusfm** upgrade.

    After successful initialization, **cmusfm** will store your Last.fm
    credentials in the ``~/.cmusfm/cmus/cmusfm.conf`` configuration file.
    See the FILES_ section for details.

    As a final step it is necessary to set **cmusfm** as a status display
    program for ``cmus(1)``. This can be done by starting **cmus** and typing
    in the main window:

    ``:set status_display_program=cmusfm``

server
    Manually run **cmusfm** server.

    This command is useful when you want to debug **cmusfm**. During normal
    operation, **cmusfm** server is started automatically, so you don't need
    to run this command.


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
