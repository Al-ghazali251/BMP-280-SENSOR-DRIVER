CC = gcc
CFLAGS = -Wall -Wextra -I.

TARGET = bmp280_test

SRC = main.c bmp280.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
