use Test::Nginx::Socket 'no_plan';

BEGIN {
    $ENV{TEST_NGINX_NO_CLEAN} = 1;
    $ENV{TEST_NGINX_USE_HUP} = 1;
}

repeat_each(2);
# plan tests => repeat_each() * (3 * blocks());
# no_shuffle();
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
    index index.html;
}
"

--- request
GET /1.updown

--- response_body eval
"1"

--- error_code: 200
