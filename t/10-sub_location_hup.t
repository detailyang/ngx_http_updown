BEGIN {
    $ENV{TEST_NGINX_USE_HUP} = 1;
    $ENV{TEST_NGINX_NO_CLEAN} = 1;
}

use Test::Nginx::Socket 'no_plan';

# plan tests => repeat_each() * (3 * blocks());
no_shuffle();
run_tests();

__DATA__
=== TEST 1: status should be ok when reload step 1
--- user_files eval
[
    [ "2.updown" => "0"],
    [ "3.updown" => "0"],
]

--- config eval
"
location = /3 {
    updown 3;
    updown_file $ENV{WORKDIR}/t/servroot/html/3.updown;
}
location = /3.updown {
    root html;
    index 3.updown;
}
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
location = /2 {
    updown 2;
    updown_file $ENV{WORKDIR}/t/servroot/html/2.updown;
}
location = /2.updown {
    root html;
    index 2.updown;
}
"

--- request eval
["GET /1", "GET /2", "GET /3"]

--- response_body eval
["up", "down", "down"]

--- error_code eval
[200, 500, 500]

=== TEST 2: status should be ok when reload step 2
--- config eval
"
location = /2 {
    updown 2;
    updown_file $ENV{WORKDIR}/t/servroot/html/2.updown;
}
location = /2.updown {
    root html;
    index 2.updown;
}
"

--- request eval
["GET /2"]

--- response_body eval
["down"]

--- error_code eval
[500]
