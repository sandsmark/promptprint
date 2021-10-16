promptprint
-----------

![screenshot](/screenshot.png)


Dependencies
-------------

 - libgit2
 - C compiler


Description
------------

My bash prompt was getting complex, so I decided to turn it into a tiny c program.

It displays an elided current path, timestamp, git status (branch, detachment, dirtiness,
ahead/behind remote, etc.), color indicating whether you're running as root or a normal user.


Usage
-----

Because of how dumb bash is, you'll have to update PS1 from PROMPTCOMMAND, e.
g. put this in your .bashrc (assuming you don't have anything else there, which
you should):

```
declare -x PROMPT_COMMAND='PS1="$(~/src/promptprint/promptprint)"'
```
