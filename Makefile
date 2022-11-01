LLVMHome = /home/linuxbrew/.linuxbrew/Cellar/llvm/15.0.3
Compiler = $(LLVMHome)/bin/clang++
IncludeDir = $(LLVMHome)/include
LinkDir = $(LLVMHome)/lib
CompilerFlags := -Wall -ggdb -Werror -std=c++20 -I$(IncludeDir) -L$(LinkDir)

client := client/main
server := server/main
programs := $(client) $(server)
srcs := $(addprefix src/, $(programs))
outs := $(addprefix bin/, $(programs))

bin/%: src/%.cc
	mkdir -p `dirname $(@)`
	$(Compiler) $(CompilerFlags) -o $(@) $<

select: %: src/client/main.cc src/server/main.cc
	echo "select unimplemented."

epoll: %: src/client/main.cc src/server/main.cc
	echo "epoll unimplemented."

uring: %: src/client/main.cc src/server/main.cc
	echo "io-uring unimplemented."

all: $(outs)

clean:
	rm -rf bin
