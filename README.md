# nginx-accept-language-module

[![Build & Test][build-test-badge]][build-test-link]
[![FreeBSD][freebsd-badge]][freebsd-link]
[![A/UBSan][sanitizers-badge]][sanitizers-link]
[![Valgrind][valgrind-badge]][valgrind-link]
[![Reload][reload-badge]][reload-link]
[![CodeQL][codeql-badge]][codeql-link]
[![Lint][lint-badge]][lint-link]

[build-test-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/build-test.yml/badge.svg
[build-test-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/build-test.yml
[freebsd-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/freebsd.yml/badge.svg
[freebsd-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/freebsd.yml
[sanitizers-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/sanitizers.yml/badge.svg
[sanitizers-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/sanitizers.yml
[valgrind-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/valgrind.yml/badge.svg
[valgrind-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/valgrind.yml
[reload-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/reload.yml/badge.svg
[reload-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/reload.yml
[codeql-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/codeql.yml/badge.svg
[codeql-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/codeql.yml
[lint-badge]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/lint.yml/badge.svg
[lint-link]: https://github.com/sysadmin-labs/nginx-accept-language-module/actions/workflows/lint.yml

Pick a language for the request out of a list the server can serve, from
the client's `Accept-Language` header, into an nginx variable.

```nginx
http {
    set_from_accept_language $lang de en fr;

    server {
        location / {
            root /var/www/$lang;
        }
    }
}
```

## The directive

### set\_from\_accept\_language

**syntax:** `set_from_accept_language $variable <language> [<language> ...]`
**context:** `http`, `server`, `location`

Defines `$variable` and gives it the language from the list that the
request asks for.  At least one language is required; the first one is what
the server prefers.

## How the language is picked

The header is a list of candidates, each with an optional weight:
`Accept-Language: de;q=0.1, en;q=0.9` asks for English and would settle for
German.

- **The weight decides.**  Among the candidates that match the list, the
  one with the highest `q` wins.  A candidate without `q` weighs 1, which
  is the most there is.  Three decimals are read, so `q=0.334` beats
  `q=0.333`.
- **`q=0` means no.**  A candidate the client refuses is never chosen.
- **On equal weight the list decides**, not the order of the header: the
  order a client writes its candidates in says nothing about preference,
  the weight does.  Where the client expressed none, the server's own
  preference is the next best thing.
- **A region covers its language.**  A configured `de` matches `de` and
  `de-AT`, and not `denmark`: the match has to end at the end of the tag or
  at a hyphen.
- **`*` speaks for the first language on the list**, which is the server's
  own choice, and it carries its weight like any other candidate.
- **Anything else answers with the first language on the list**: no header,
  nothing acceptable, or nothing that matches.
- Case is ignored, as are spaces and tabs around the candidates.  A
  parameter other than `q` is stepped over.  A weight that is not a number
  leaves the candidate at 1: a client that writes nonsense into one
  parameter still said which languages it wants.

## One variable per http block

nginx variables are per `http` block, so `set_from_accept_language` with
the same variable twice -- once per `server`, say -- is refused with
"variable already defined", although the directive is allowed in `server`
and `location`.  One variable, one list; two lists need two variables.

## Building

```sh
ci/build.sh 1.30.5 /tmp/nginx-test            # into the binary
ci/build.sh 1.30.5 /tmp/nginx-test dynamic    # as a loadable object
```

## Testing

```sh
ci/build.sh 1.30.5 /tmp/nginx-test
TEST_NGINX_BINARY=/tmp/nginx-test/sbin/nginx prove -r t/
```

`t/quality.t` is the one that matters: thirteen headers whose answer is
decided by the weights, which is what the module this was taken over from
threw away.

## License

BSD 2-Clause.  The module is Guillaume Maury's; the weighting and the
scaffolding around it are under the same terms.  See [LICENSE](LICENSE).
