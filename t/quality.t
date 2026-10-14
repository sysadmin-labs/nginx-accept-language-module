# vi:ft=perl
#
# The weights, which is the whole reason this repository exists: the module
# this was taken over from threw the q parameter away and answered with the
# first candidate it found a match for, so a client that asked for English
# and would settle for German got German.
#
# q travels in thousandths here, so "q=0.95" is 950 and these blocks are
# exact rather than approximate.

use Test::Nginx::Socket 'no_plan';

repeat_each(2);

run_tests();

__DATA__

=== TEST 1: the heavier weight wins, not the earlier mention
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;q=0.1, en;q=0.9
--- request
GET /l
--- response_body chomp
en



=== TEST 2: q=0 means the client does not want it
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;q=0, en
--- request
GET /l
--- response_body chomp
en



=== TEST 3: everything refused leaves the server's first choice
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;q=0, en;q=0
--- request
GET /l
--- response_body chomp
de



=== TEST 4: the same weight, and the list breaks the tie
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=0.5, de;q=0.5
--- request
GET /l
--- response_body chomp
de



=== TEST 5: the same tie, the other list
--- config
    set_from_accept_language $lang en de;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=0.5, de;q=0.5
--- request
GET /l
--- response_body chomp
en



=== TEST 6: three decimals are read, and they decide
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;q=0.333, en;q=0.334
--- request
GET /l
--- response_body chomp
en



=== TEST 7: a missing q is the highest weight there is
--- config
    set_from_accept_language $lang en de;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=0.9, de
--- request
GET /l
--- response_body chomp
de



=== TEST 8: q=1.000 and no q are the same thing, so the list decides
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=1.000, de
--- request
GET /l
--- response_body chomp
de



=== TEST 9: a parameter this does not know is stepped over
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;foo=bar;q=0.1, en;q=0.2
--- request
GET /l
--- response_body chomp
en



=== TEST 10: a weight that is not a number leaves the default
# A client that writes nonsense into one parameter still said which
# languages it wants.  Dropping its candidate would answer in the wrong
# language rather than in none, so the weight stays at 1.
--- config
    set_from_accept_language $lang en de;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;q=abc, en;q=0.9
--- request
GET /l
--- response_body chomp
de



=== TEST 11: an upper-case Q is a q
--- config
    set_from_accept_language $lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: de;Q=0.1, en;Q=0.9
--- request
GET /l
--- response_body chomp
en



=== TEST 12: a star carries its weight too
# The star stands for the server's first choice, and here it outweighs the
# language the client named.
--- config
    set_from_accept_language $lang fr en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=0.1, *;q=0.9
--- request
GET /l
--- response_body chomp
fr



=== TEST 13: and loses when the named language weighs more
--- config
    set_from_accept_language $lang fr en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en;q=0.9, *;q=0.1
--- request
GET /l
--- response_body chomp
en
