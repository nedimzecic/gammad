CC = gcc
CFLAGS = -Wall -Wextra -O2 -I/usr/include/libdrm
LDFLAGS = -ldrm -lm
TARGET = gammad
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

debug: CFLAGS = -Wall -Wextra -g -O0 -I/usr/include/libdrm
debug: clean $(TARGET)

help:
	@echo "Available targets:"
	@echo "  all      - Build the gammad binary (default)"
	@echo "  clean    - Remove object files and binary"
	@echo "  install  - Install gammad to /usr/local/bin (requires sudo)"
	@echo "  uninstall- Remove gammad from /usr/local/bin"
	@echo "  debug    - Build with debug symbols"
	@echo "  help     - Show this help message"

.PHONY: all clean install uninstall debug help
