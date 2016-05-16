PATH := /usr/local/bin:/usr/local/nginx/sbin:$(PATH)
test:
	@WORKDIR=$(shell pwd) /usr/bin/prove

.PHONY: test
