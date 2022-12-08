# variables
CC = gcc -Wall

PROJECT_DIR=.
SRC_DIR=./src
BIN_DIR=./bin

# CI/CD
clean:
	rm -rf bin "$(PROJECT_DIR)/Receiver" "$(PROJECT_DIR)/Sender"

# units
$(BIN_DIR):
	mkdir $(BIN_DIR)

$(BIN_DIR)/Receiver.o: $(BIN_DIR) $(SRC_DIR)/Receiver.c
	$(CC) -o "$(BIN_DIR)/Receiver.o" -c "$(SRC_DIR)/Receiver.c"

$(BIN_DIR)/Sender.o: $(BIN_DIR) $(SRC_DIR)/Sender.c
	$(CC) -o "$(BIN_DIR)/Sender.o" -c "$(SRC_DIR)/Sender.c"

# applications
Receiver: $(BIN_DIR)/Receiver.o
	$(CC) -o "$(PROJECT_DIR)/Receiver" "$(BIN_DIR)/Receiver.o"

Sender: $(BIN_DIR)/Sender.o
	$(CC) -o "$(PROJECT_DIR)/Sender" "$(BIN_DIR)/Sender.o"

# prod
build: Receiver Sender

rebuild: clean build