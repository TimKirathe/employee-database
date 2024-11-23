TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbclient

SRC_SRV = $(wildcard src/srv/*.c)
# OBJ_SRV = $(SRC_SRV:src/srv/*.c=src/srv/*.o)
OBJ_SRV = $(patsubst src/srv/%.c, obj/srv/%.o, $(SRC_SRV))

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(patsubst src/cli/%.c, obj/cli/%.o, $(SRC_CLI))


run: clean default
	# ./$(TARGET_SRV) -f ./mynewdb.db -n &
	# ./$(TARGET_CLI) 100.68.24.180
	# kill -9 $$(pidof dbserver)

default: $(TARGET_SRV) $(TARGET_CLI)

clean:
	rm -f obj/srv/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
	gcc -g -o $@ $?

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	gcc -g -c $< -o $@ -Iinclude

$(TARGET_CLI): $(OBJ_CLI)
	gcc -g -o $@ $?

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	gcc -g -c $< -o $@ -Iinclude


