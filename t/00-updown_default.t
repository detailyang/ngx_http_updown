use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: updown_default should be down
--- config
location = /1 {
	updown 1;
    updown_default down;
}
--- request
GET /1
--- response_body eval
"down"
--- error_code: 500


=== TEST 2: updown_default should be up
--- config
location = /1 {
    updown 1;
    updown_default up;
}
--- request
GET /1
--- response_body eval
"up"
--- error_code: 200
