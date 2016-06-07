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
--- config
location = /1 {
    updown 1;
}

--- request
DELETE /1

--- response_body eval
["down"]

--- error_code: 200

=== TEST 2: status should be ok when reload step 2
--- config
location = /1 {
    updown 1;
}
--- request
GET /1

--- response_body eval
["total:down updown:down upstream:disable"]

--- error_code: 500

=== TEST 3: status should be ok when reload step 3
--- config
location = /1 {
    updown 1;
}
--- request
POST /1

--- response_body eval
["up"]

--- error_code: 200

=== TEST 4: status should be ok when reload step 4
--- config
location = /1 {
    updown 1;
}
--- request
GET /1

--- response_body eval
["total:up updown:up upstream:disable"]

--- error_code: 200

=== TEST 5: status should be ok when reload step 5
--- config
location = /1 {
    updown 1;
}

--- request
DELETE /1

--- response_body eval
["down"]

--- error_code: 200

=== TEST 6: status should be ok when reload step 5
--- config
location = /1 {
    updown 1;
}

--- request
GET /1

--- response_body eval
["total:down updown:down upstream:disable"]

--- error_code: 500
