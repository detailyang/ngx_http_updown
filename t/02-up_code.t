use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: up_code should be 201
--- config
location = /1 {
    updown 1;
    up_code 201;
    updown_default up;
}
--- request
GET /1
--- response_body eval
"up"
--- error_code: 201


=== TEST 2: up_code should be 501
--- config
location = /1 {
    updown 1;
    up_code 501;
    updown_default up;
}
--- request
GET /1
--- response_body eval
"up"
--- error_code: 501
