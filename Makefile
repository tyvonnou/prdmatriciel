CC=gcc
CLFAGS=-W -Wall -pedantic -gnu99
LDFLAGS=-pthread

all: RecupDonnees produit.out

RecupDonnees: objects/donnees.o
	$(CC) $^ -o $@ $(LDFLAGS)

produit.out: objects/produit.o
	$(CC) $^ -o $@ $(LDFLAGS)

objects/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm objects/*.o
	-rm *.out
