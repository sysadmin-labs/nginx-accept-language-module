# vi:ft=perl
#
# What the variable answers for a given Accept-Language, without q coming
# into it.  t/quality.t has the weights, t/config.t what is refused.
#
# Every block asks the same thing: a location that answers with the
# variable, so the body is the module's decision and nothing else.

use Test::Nginx::Socket 'no_plan';

repeat_each(2);

run_tests();

__DATA__

=== TEST 1: no header at all answers with the first of the list
--- config
    set_from_accept_language $lang de en fr;

    location = /l {
        return 200 $lang;
    }
--- request
GET /l
--- response_body chomp
de



=== TEST 2: the language the client asked for
--- config
    set_from_accept_language $lang de en fr;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: fr
--- request
GET /l
--- response_body chomp
fr



=== TEST 3: a region covers the language it belongs to
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de-AT
--- request
GET /l
--- response_body chomp
de



=== TEST 4: a longer word that merely starts the same way is not a match
# "denmark" begins with "de" and means nothing of the sort.  The list has
# "en" first here, so the answer tells a match apart from the fallback.
--- config
    set_from_accept_language $lang en de;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: denmark
--- request
GET /l
--- response_body chomp
en



=== TEST 5: case does not matter
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: EN-GB
--- request
GET /l
--- response_body chomp
en



=== TEST 6: spaces and tabs around the candidates and before the weight
# The weight has to decide this one, not the list: if the spaces ended up
# inside the tag, "fr" would match nothing and the answer would be "de" --
# which is also what the list alone would have produced.
--- config
    set_from_accept_language $lang de en fr;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: 	 ,  fr ;q=0.9 ,de;q=0.5
--- request
GET /l
--- response_body chomp
fr



=== TEST 7: a star speaks for the server's own first choice
--- config
    set_from_accept_language $lang fr en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: *
--- request
GET /l
--- response_body chomp
fr



=== TEST 8: two candidates without weights, the list decides
# The order of the header says nothing about preference, so the server's
# does: "de" comes first in the list and wins although "en" was named
# first in the request.
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en, de
--- request
GET /l
--- response_body chomp
de



=== TEST 9: nothing in the header is on the list
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: ja, ko
--- request
GET /l
--- response_body chomp
de
