BEGIN {
    $ENV{TEST_NGINX_USE_HUP} = 1;
    $ENV{TEST_NGINX_NO_CLEAN} = 1;
}

use Test::Nginx::Socket 'no_plan';

repeat_each(2);
# plan tests => repeat_each() * (3 * blocks());
no_shuffle();
run_tests();

__DATA__
=== TEST 1: status should be ok when reload step 1
--- config eval
"
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request eval
["GET /1", "GET /1.updown"]

--- response_body eval
["up", "1"]

--- error_code eval
[200, 200]

=== TEST 2: status should be ok when reload step 2
--- config eval
"
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request eval
["DELETE /1", "GET /1.updown", "POST /1", "GET /1.updown"]

--- response_body eval
["down", "0", "up", "1"]

--- error_code eval
[200, 200, 200, 200]

=== TEST 3: status should be ok when reload step 3
--- config eval
"
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request eval
["DELETE /1", "GET /1.updown",
 "POST /1", "GET /1.updown", "DELETE /1", "GET /1.updown"]

--- response_body eval
["down", "0",
 "up", "1",
 "down", "0"]

--- error_code eval
[200, 200,
 200, 200,
 200, 200]


=== TEST 4: status should be ok when reload step 4
--- user_files eval
[
    [ "1.updown" => "0"],
]

--- config eval
"
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request
GET /1.updown

--- response_body eval
"0"

--- error_code: 200

=== TEST 4: status should be ok when reload step 5
--- config eval
"
location = /1 {
    updown 1;
    updown_file $ENV{WORKDIR}/t/servroot/html/1.updown;
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request
GET /1

--- response_body eval
"down"

--- error_code: 500
