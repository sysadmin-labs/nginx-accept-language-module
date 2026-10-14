# Changelog

Newest first.  Dates are release dates.

## v1.1.0 (2026-10-14)

### Changed

- **The weight in `Accept-Language` decides which language is picked.**
  What this was taken over from read the header from left to right and
  answered with the first candidate it found on the list, with a comment
  saying so: *"We discard the quality value"*.  A client sending
  `de;q=0.1, en;q=0.9` -- English, and German if it has to be -- was
  answered in German.  Now the heaviest matching candidate wins, `q=0`
  means the client refuses that language, and on equal weight the server's
  list decides, because the order a client writes its candidates in says
  nothing about preference while the weight does.

  This changes what an existing configuration answers, and that is the
  point of it.  A header without weights answers as before.

### Fixed

- **An uninitialised read, reachable with one line of configuration.**
  `set_from_accept_language $lang;` was accepted -- the directive took "one
  argument or more" -- and left the list empty, while the array had room
  for one element that nothing had written to.  The variable then answered
  with whatever that memory held, at whatever length, which is what ends up
  in a log line or a response.  The directive takes two arguments or more
  now, so the configuration is refused while it is read.

- **The module compiled only against an nginx that happened to define
  `NGX_HTTP_HEADERS`.**  It read `r->headers_in.accept_language`, a member
  that exists only in that case, and the original upstream defined the
  macro in its own `config` to make it appear.  That works for a module
  built into the binary and not for a loadable one, where the shape of the
  structure was decided when that binary was compiled -- and the fork this
  was taken over from had dropped the line while adding the loadable
  build.  The header is looked up in `r->headers_in.headers` now, which
  needs nothing from the build.

- **A configured language matched any word that started with it.**  `de`
  answered for `denmark`, `en` for `english`, because the comparison looked
  at the configured length and stopped there.  The match now has to end at
  the end of the tag or at a hyphen, so `de` still covers `de-AT`.

- `*` is a candidate like any other and carries its weight.  It stands for
  the first language on the list, which is the server's own preference.

- The scan read one byte past the header value before testing the bound.
  It was harmless, because nginx writes a NUL after every header value, but
  the test came after the read.

- An empty variable name is refused instead of being handed to
  `ngx_http_add_variable`.

### Added

- `t/`, in three files: what the variable answers for a given header,
  thirteen blocks whose answer is decided by the weights, and what is
  refused while the configuration is read.

- The reload gate asks the module rather than the server: the probe sends
  three weighted candidates and insists on the one that only a module
  weighing them can produce.

- The scaffolding the other modules in this organization carry: a version
  and digest pin for every build input, the release matrix with both a
  static and a loadable build, the FreeBSD virtual machine, A/UBSan,
  valgrind, eight reloads in a row, CodeQL, shellcheck and actionlint.

### Removed

- The upstream `README.md`, whose installation section was about
  `./configure --add-module` against an nginx source tree by hand.
