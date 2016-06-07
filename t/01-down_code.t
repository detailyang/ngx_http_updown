use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: down_code should be 201
--- config
location = /1 {
    updown 1;
    down_code 201;
    updown_default down;
}
--- request
GET /1
--- response_body eval
"total:down updown:down upstream:disable"
--- error_code: 201


=== TEST 2: down_code should be 501
--- config
location = /1 {
    updown 1;
    down_code 501;
    updown_default down;
}
--- request
GET /1
--- response_body eval
"total:down updown:down upstream:disable"
--- error_code: 501
