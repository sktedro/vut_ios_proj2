CFILE=proj2.c
LIBS=proj2.h
PROGDEST=proj2
ZIPDEST=proj2.zip

default: all

all:
	gcc -std=gnu99 $(CFILE) $(LIBS) -Wall -Wextra -Werror -pedantic -o $(PROGDEST) -pthread -lrt -g

run: all
	./$(PROGDEST) 5 5 10 10

zip: clean
	zip $(ZIPDEST) ./*

clean:
	rm -f $(ZIPDEST) $(PROGDEST)
