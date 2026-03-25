CC := emcc
CXX := em++
FLAGS := -O3 -Wall -I..
CFLAGS := -std=c11 $(FLAGS)
CXXFLAGS := -std=c++11 $(FLAGS)
EMFLAGS := -s USE_ZLIB=1 -s DISABLE_EXCEPTION_CATCHING=1 -s INVOKE_RUN=0 -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0
EXPORTS := -s EXPORTED_RUNTIME_METHODS='["callMain","cwrap"]' -s EXPORTED_FUNCTIONS='["_main","_paintLCD","_malloc"]'
PREJS := --pre-js firebird-utils.js
LFLAGS := -s INITIAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1
OUTPUT := firebird

CSOURCES :=    ../core/armsnippets_loader.c ../core/asmcode.c ../core/casplus.c ../core/des.c ../core/disasm.c \
	      ../core/gdbstub.c ../core/interrupt.c ../core/lcd.c ../core/link.c ../core/mem.c \
	      ../core/misc.c ../core/mmu.c ../core/schedule.c ../core/serial.c ../core/sha256.c ../core/usb.c \
              ../core/usblink.c ../core/os/os-emscripten.c

CPPSOURCES := ../core/arm_interpreter.cpp ../core/coproc.cpp ../core/cpu.cpp ../core/debug.cpp ../core/emu.cpp \
	      ../core/flash.cpp ../core/gif.cpp ../core/thumb_interpreter.cpp ../core/usblink_queue.cpp ../core/keypad.cpp main.cpp \
	      ../core/fieldparser.cpp

OBJS = $(patsubst %.c, %.bc, $(CSOURCES))
OBJS += $(patsubst %.cpp, %.bc, $(CPPSOURCES))

all: $(OUTPUT).js

%.bc: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.bc: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(OUTPUT).js
	emrun --safe_firefox_profile --browser firefox $(OUTPUT).html

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(EMFLAGS) $(EXPORTS) $(PREJS) $(LFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(OUTPUT).js $(OUTPUT).data $(OUTPUT).js.mem
