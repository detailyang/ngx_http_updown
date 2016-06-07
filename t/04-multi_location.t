use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: multi location should be up
--- config
location = /1 {
    updown 1;
}
location = /2 {
    updown 2;
}
location = /3 {
    updown 3;
}
--- pipelined_requests eval
["GET /1", "GET /2", "GET /3"]

--- response_body eval
["total:up updown:up upstream:disable", "total:up updown:up upstream:disable", "total:up updown:up upstream:disable"]
--- error_code eval
[200, 200, 200]

=== TEST 2: multi location should be isolated
--- config
location = /1 {
    updown 1;
}
location = /2 {
    updown 2;
}
location = /3 {
    updown 3;
}

--- request eval
["GET /1", "GET /2", "GET /3",
 "DELETE /1",
 "GET /1", "GET /2", "GET /3",
 "DELETE /2",
 "GET /1", "GET /2", "GET /3",
 "DELETE /3",
 "GET /1", "GET /2", "GET /3",
 "POST /2",
 "GET /1", "GET /2", "GET /3",
]

--- response_body eval
["total:up updown:up upstream:disable", "total:up updown:up upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:up updown:up upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:down updown:down upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:down updown:down upstream:disable", "total:down updown:down upstream:disable",
 "up",
 "total:down updown:down upstream:disable", "total:up updown:up upstream:disable", "total:down updown:down upstream:disable"
]
--- error_code eval
[200, 200, 200,
 200,
 500, 200, 200,
 200,
 500, 500, 200,
 200,
 500, 500, 500,
 200,
 500, 200, 500,
]

=== TEST 3: multi location updown file should be isolated
--- config eval
"
location = /1 {
    updown 1;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
location = /1.updown {
    root html;
    index 1.updown;
}
location = /2 {
    updown 2;
    updown_file '$ENV{WORKDIR}/t/servroot/html/2.updown';
}
location = /2.updown {
    root html;
    index 2.updown;
}
location = /3 {
    updown 3;
    updown_file '$ENV{WORKDIR}/t/servroot/html/3.updown';
}
location = /3.updown {
    root html;
    index 3.updown;
}
"

--- request eval
["GET /1", "GET /2", "GET /3",
 "DELETE /1",
 "GET /1", "GET /2", "GET /3",
 "DELETE /2",
 "GET /1", "GET /2", "GET /3",
 "DELETE /3",
 "GET /1", "GET /2", "GET /3",
 "POST /2",
 "GET /1", "GET /2", "GET /3",
 "GET /1.updown", "GET /2.updown", "GET /3.updown"
]

--- response_body eval
["total:up updown:up upstream:disable", "total:up updown:up upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:up updown:up upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:down updown:down upstream:disable", "total:up updown:up upstream:disable",
 "down",
 "total:down updown:down upstream:disable", "total:down updown:down upstream:disable", "total:down updown:down upstream:disable",
 "up",
 "total:down updown:down upstream:disable", "total:up updown:up upstream:disable", "total:down updown:down upstream:disable",
 "0", "1", "0"
]

--- error_code eval
[200, 200, 200,
 200,
 500, 200, 200,
 200,
 500, 500, 200,
 200,
 500, 500, 500,
 200,
 500, 200, 500,
 200, 200, 200,
]

=== TEST 4: multi location updown file when file exists should be isolated
--- user_files eval
[
    [ "1.updown" => "0"],
    [ "2.updown" => "0"],
    [ "3.updown" => "0"],
]

--- config eval
"
location = /1 {
    updown 1;
    updown_file '$ENV{WORKDIR}/t/servroot/html/1.updown';
}
location = /1.updown {
    root html;
    index 1.updown;
}
location = /2 {
    updown 2;
    updown_file '$ENV{WORKDIR}/t/servroot/html/2.updown';
}
location = /2.updown {
    root html;
    index 2.updown;
}
location = /3 {
    updown 3;
    updown_file '$ENV{WORKDIR}/t/servroot/html/3.updown';
}
location = /3.updown {
    root html;
    index 3.updown;
}
"

--- request eval
["GET /1", "GET /2", "GET /3"]

--- response_body eval
["total:down updown:down upstream:disable", "total:down updown:down upstream:disable", "total:down updown:down upstream:disable"]

--- error_code eval
[500, 500, 500]

