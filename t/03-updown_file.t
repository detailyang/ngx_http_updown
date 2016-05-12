use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: updown_file default should be up
--- config eval
"
location = /1 {
    updown;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
location = /1.updown {
    root html;
    index 1.updown;
}
"
--- pipelined_requests eval
["GET /1.updown", "GET /1"]

--- response_body eval
["1", "up"]

--- error_code eval
[200, 200]


=== TEST 2: updown_file should be up
--- user_files
>>> 1.updown
1
--- config eval
"
location = /1 {
    updown;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
"
--- request
GET /1
--- response_body eval
"up"
--- error_code: 200

=== TEST 3: updown_file should be down
--- user_files
>>> 1.updown
0
--- config eval
"
location = /1 {
    updown;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
"
--- request
GET /1
--- response_body eval
"down"
--- error_code: 500

=== TEST 4: updown_file should be down
--- user_files
>>> 1.updown
0
--- config eval
"
location = /1 {
    updown;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
"
--- request
GET /1
--- response_body eval
"down"
--- error_code: 500

=== TEST 5: updown_file should be created
--- config eval
"
location = /1 {
    updown;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- request
GET /1.updown

--- error_code: 200

=== TEST 6: updown_file should be down
--- config eval
"
location = /1 {
    updown;
    updown_default down;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
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

=== TEST 7: updown_file should keep track on updown status
--- config eval
"
location = /1 {
    updown;
    updown_default down;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
location = /1.updown {
    root html;
    index 1.updown;
}
"

--- pipelined_requests eval
["GET /1.updown", "GET /1", "POST /1", "GET /1", "GET /1.updown", "DELETE /1", "GET /1", "GET /1.updown"]

--- response_body eval
["0", "down", "up", "up", "1", "down", "down", "0"]

--- error_code eval
[200, 500, 200, 200, 200, 200, 500, 200]
