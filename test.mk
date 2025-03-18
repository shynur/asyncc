#! /bin/make -f

.PHONY: AsyncManualResetEvent
AsyncManualResetEvent: bin/TestAsyncManualResetEvent.exe
	@./$<

bin/%.exe: src/Test%.cpp include/%.hpp
	@mkdir -p bin
	clang++-21 -std=c++26 -o $@ -Iinclude $<

.PHONY: clean
clean:
	rm -rf bin/
