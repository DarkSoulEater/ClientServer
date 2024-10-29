all: shared client server

client:
	@cd Client && make
	@mkdir -p bin
	@cp Client/build-debug/* bin/

server:
	@cd Server && make
	@mkdir -p bin
	@cp Server/build-debug/* bin/

shared:
	@cd Shared && make