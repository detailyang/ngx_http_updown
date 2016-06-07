BEGIN {
    $ENV{TEST_NGINX_USE_HUP} = 1;
    $ENV{TEST_NGINX_CHECK_LEAK} = 1;
}

use Test::Nginx::Socket 'no_plan';

repeat_each(10);
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
["GET /1", "GET /1.updown", "DELETE /1", "POST /1"]

--- response_body eval
["total:up updown:up upstream:disable", "1", "total:down updown:down upstream:disable", "total:up updown:up upstream:disable"]

--- error_code eval
[200, 200, 200, 200]

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
["GET /1", "GET /1.updown", "DELETE /1", "POST /1",
 "GET /2", "GET /2.updwon", "DELETE /2", "POST /2"
]

--- response_body eval
["total:up updown:up upstream:disable", "1", "total:down updown:down upstream:disable", "total:up updown:up upstream:disable",
 "total:up updown:up upstream:disable", "1", "total:down updown:down upstream:disable", "total:up updown:up upstream:disable"
]

--- error_code eval
[200, 200, 200, 200,
 200, 200, 200, 200
]
