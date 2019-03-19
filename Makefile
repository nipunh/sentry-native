clean:
	rm -rf example libsentry.* crashpad-db *.dSYM *.mp

crashpad-mac:
	g++ -g -dynamiclib \
		-o libsentry.dylib src/sentry.cpp src/crashpad_wrapper.cpp src/vendor/mpack.c \
		-I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ \
		-I ./include \
		-fvisibility=hidden \
		-std=c++14 -L../crashpad-Darwin/lib -lclient -lbase -lutil \
		-framework Foundation -framework Security -framework CoreText \
		-framework CoreGraphics -framework IOKit -lbsm \
		-D SENTRY_CRASHPAD
example-crashpad-mac: example.c libsentry.dylib
	gcc -g -o example example.c -I ./include -L . -lsentry

breakpad-mac:
	g++ -g -dynamiclib \
		-o libsentry.dylib src/sentry.cpp src/breakpad_wrapper.cpp src/vendor/mpack.c \
		-I ../breakpad-Darwin/include/ \
		-I ./include \
		-fvisibility=hidden \
		-L ../breakpad-Darwin/lib \
		-lbreakpad_client -lpthread \
		-std=c++11 \
		-framework Foundation \
		-D SENTRY_BREAKPAD
example-breakpad-mac: example.c libsentry.dylib
	gcc -g -o example example.c -I ./include -L . -lsentry

breakpad-linux:
	clang++ -g -shared \
		-fPIC \
		-o libsentry.so src/sentry.cpp src/breakpad_wrapper.cpp src/vendor/mpack.c \
		-I ../breakpad-Linux/include/ \
		-I ./include \
		-fvisibility=hidden \
		-L ../breakpad-Linux/lib \
		-lbreakpad_client -lpthread \
		-std=c++14 \
		-D SENTRY_BREAKPAD
example-breakpad-linux: example.c libsentry.dylib
	gcc -g -o example example.c -I ./include -L . -lsentry