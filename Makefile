Compiler = /usr/local/opt/llvm/bin/clang++
IncludeDir = /usr/local/opt/llvm/include
LinkDir = /usr/local/opt/llvm/lib
CompilerFlags := -Wall -ggdb -Werror -std=c++20 -I$(IncludeDir) -L$(LinkDir)

Poll := nw_poll
Select := nw_select
EPoll := nw_epoll
URing := nw_ring

all: $(Poll) $(Select) $(EPoll) $(URing)

$(Poll): %: src/client/main.cc src/server/main.cc
	$(Compiler) $(CompilerFlags) -o $@ $<

$(Select): %: %.cc
	echo "select unimplemented."

$(Epoll): %: %.cc
	echo "epoll unimplemented."

$(URing): %: %.c
	echo "io-uring unimplemented."

clean:
	rm -f $(Poll) $(Select) $(EPoll) $(URing)
