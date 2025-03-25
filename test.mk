#! /bin/make -f

.PHONY: %
%: bin/Test%.exe | bin/
	@./$<

bin/Test%.exe: src/Test%.cpp include/asyncxx/%.hpp | bin/
	clang++-21 --gcc-install-dir=/usr/local/lib/gcc/x86_64-pc-linux-gnu/15.0.1  \
	  -std=c++26 -Wpedantic -Wall -W -Wno-exceptions  \
	  -O0 -g3 -fsanitize=address,undefined,leak  \
	  -o $@ -Iinclude $<

%/:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -rf bin/
