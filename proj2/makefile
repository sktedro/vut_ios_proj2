CFILE=proj2.c
LIBS=proj2.h
PROGDEST=proj2
ZIPDEST=proj2.zip

default: all

all:
	gcc -std=gnu99 $(CFILE) $(LIBS) -Wall -Wextra -Werror -pedantic -o $(PROGDEST) -pthread -lrt -g

run: all
	#./$(PROGDEST) 5 5 10 10
	./$(PROGDEST) 20 5 10 10

test: all
	cp p2test/p2test.py ./
	-python3 p2test.py
	rm ./p2test.py

zip: clean
	zip $(ZIPDEST) ./*

clean:
	rm -f $(ZIPDEST) $(PROGDEST)
