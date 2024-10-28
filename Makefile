all: shared client server
	@mkdir -p bin
	@cp Client/build-debug/* bin/
	@cp Server/build-debug/* bin/

client:
	@cd Client && make

server:
	@cd Server && make

shared:
	@cd Shared && make