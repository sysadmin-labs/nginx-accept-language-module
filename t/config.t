# vi:ft=perl
#
# What is refused while the configuration is read.  The first block is the
# reason the arity changed: the module this was taken over from took the
# variable name alone, and then answered with whatever the uninitialised
# first element of an empty list happened to hold.

use Test::Nginx::Socket 'no_plan';

repeat_each(1);

run_tests();

__DATA__

=== TEST 1: a variable and no language at all
--- config
    set_from_accept_language $lang;

    location = /l {
        return 200 $lang;
    }
--- must_die
--- error_log
invalid number of arguments in "set_from_accept_language" directive



=== TEST 2: nothing at all
--- config
    set_from_accept_language;

    location = /l {
        return 200 'x';
    }
--- must_die
--- error_log
invalid number of arguments in "set_from_accept_language" directive



=== TEST 3: an empty variable name
--- config
    set_from_accept_language "" de en;

    location = /l {
        return 200 'x';
    }
--- must_die
--- error_log
the variable name must not be empty



=== TEST 4: the same variable twice
--- config
    set_from_accept_language $lang de en;
    set_from_accept_language $lang fr it;

    location = /l {
        return 200 $lang;
    }
--- must_die
--- error_log
variable already defined: "lang"



=== TEST 5: a name without the dollar sign is taken, with a word about it
--- config
    set_from_accept_language lang de en;

    location = /l {
        return 200 $lang;
    }
--- more_headers
Accept-Language: en
--- request
GET /l
--- response_body chomp
en
--- error_log
variable name should start with '$'
