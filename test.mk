#! /bin/make -f

.PHONY: AsyncManualResetEvent
AsyncManualResetEvent: bin/TestAsyncManualResetEvent.exe | bin/
	@./$<

bin/Test%.exe: src/Test%.cpp include/asyncxx/%.hpp | bin/
	clang++-21 -std=c++26 -Wpedantic -Wall -W -g0 -O3 -o $@ -Iinclude $<

%/:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -rf bin/
