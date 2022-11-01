LLVMHome = /home/linuxbrew/.linuxbrew/Cellar/llvm/15.0.3
CC = $(LLVMHome)/bin/clang
CXX = $(LLVMHome)/bin/clang++
LibLLVM = -I$(LLVMHome)/include -L$(LLVMHome)/lib
LibUring = -Lliburing/src -Iliburing/src/include -luring
CompilerFlags := -Wall -ggdb -Werror -std=c++20 $(LibLLVM) $(LibUring)

client := client/main
server := server/main
programs := $(client) $(server)
srcs := $(addprefix src/, $(programs))
outs := $(addprefix bin/, $(programs))

all: $(outs)

bin/%: src/%.cc
	mkdir -p `dirname $(@)`
	$(CXX) $(CompilerFlags) -o $(@) $< -luring

poll:
	make $(outs)

select: %: src/client/main.cc src/server/main.cc
	echo "select unimplemented."

epoll: %: src/client/main.cc src/server/main.cc
	echo "epoll unimplemented."

uring: %: src/client/main.cc src/server/main.cc
	echo "io-uring unimplemented."

run_client:
	bin/$(client)

run_server:
	bin/$(server)

clean:
	rm -rf bin
