CC=gcc
CLFAGS=-W -Wall -pedantic -gnu99
LDFLAGS=-pthread

all: RecupDonnees

RecupDonnees: objects/donnees.o
	$(CC) $^ -o $@ $(LDFLAGS)

objects/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm objects/*.o
	-rm *.out
