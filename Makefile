CFLAGS = -W -Wall -Werror -Wshadow -Wunused -Wformat -std=c11 -pedantic -pthread -O3
SOURCES = $(wildcard *.c)
OBJECTS = $(subst .c,.o,$(SOURCES))
DEPENDS = $(subst .c,.d,$(SOURCES))
MAIN = serwer komisja raport

all: $(DEPENDS) $(MAIN)

$(DEPENDS): %.d: %.c
	$(CC) -MM $< > $@

-include $(DEPENDS)

serwer: serwer.o serwer-common.o serwer-results.o serwer-report.o
	$(CC) $(LDFLAGS) -pthread $^ -o $@

komisja: komisja.o client-common.o

raport: raport.o client-common.o

clean:
	$(RM) $(OBJECTS) $(MAIN) $(DEPENDS)

.PHONY: all clean
