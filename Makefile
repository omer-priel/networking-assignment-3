# variables
CC = gcc -Wall

PROJECT_DIR=.
SRC_DIR=./src
BIN_DIR=./bin

# CI/CD
clean:
	rm -rf bin "$(PROJECT_DIR)/Receiver" "$(PROJECT_DIR)/Sender"

install:
	cd scripts
	poetry install

create-inputs:
	rm -rf inputs
	mkdir inputs
	cd scripts && poetry run lorem_text --words 10000 > ../inputs/input.txt

# units
$(BIN_DIR):
	mkdir $(BIN_DIR)

$(BIN_DIR)/Receiver.o: $(BIN_DIR) $(SRC_DIR)/Receiver.c $(SRC_DIR)/config.h
	$(CC) -o "$(BIN_DIR)/Receiver.o" -c "$(SRC_DIR)/Receiver.c"

$(BIN_DIR)/Sender.o: $(BIN_DIR) $(SRC_DIR)/Sender.c $(SRC_DIR)/config.h
	$(CC) -o "$(BIN_DIR)/Sender.o" -c "$(SRC_DIR)/Sender.c"

# applications
Receiver: $(BIN_DIR)/Receiver.o
	$(CC) -o "$(PROJECT_DIR)/Receiver" "$(BIN_DIR)/Receiver.o"

Sender: $(BIN_DIR)/Sender.o
	$(CC) -o "$(PROJECT_DIR)/Sender" "$(BIN_DIR)/Sender.o"

# build
build: Receiver Sender

rebuild: clean build

# testing
disable-tc:
	sudo tc qdisc del dev lo root netem

tc-10:
	sudo tc qdisc add dev lo root netem loss 10%

tc-15:
	sudo tc qdisc add dev lo root netem loss 15%

tc-20:
	sudo tc qdisc add dev lo root netem loss 20%

tc-25:
	sudo tc qdisc add dev lo root netem loss 25%

tc-30:
	sudo tc qdisc add dev lo root netem loss 30%