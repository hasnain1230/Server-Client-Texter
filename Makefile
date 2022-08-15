TARGET = server
CLIENT = client
CC = gcc
CFLAGS = -g -std=c99 -Wall -pthread

OBJ = $(TARGET).o

all: $(TARGET)

run: $(TARGET)
	./$<

# ^ is all pre-reqs
# @ is target

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET).o: $(TARGET).c
	$(CC) -c $(CFLAGS) $<

$(CLIENT): $(CLIENT).o
	$(CC) $(CFLAGS) $^ -o $@

$(CLIENT).o: $(CLIENT).c
	$(CC) -c $(CFLAGS) $<

clean:
	rm -rfv $(TARGET) *.o *.a *.dylib *.dSYM