use Test::Nginx::Socket 'no_plan';


BEGIN {
    $ENV{TEST_NGINX_USE_HUP} = 1;
}

master_on();
workers(2);
run_tests();

__DATA__
=== TEST 1: up_code should be 201
--- config
location = /1 {
    updown 1;
}
--- request
GET /1
--- response_body eval
"up"
--- error_code: 200

=== TEST 2: up_code should be 201
--- config
location = /1 {
    updown 1;
}
--- request
GET /1
--- response_body eval
"up"
--- error_code: 200
