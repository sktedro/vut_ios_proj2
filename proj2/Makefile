CFILE=proj2.c
LIBS=proj2.h
PROGDEST=proj2
ZIPDEST=proj2.zip
LOGFILE=proj2.out

default: all

all:
	gcc -std=gnu99 $(CFILE) $(LIBS) -Wall -Wextra -Werror -pedantic -o $(PROGDEST) -pthread

zip:
	zip $(ZIPDEST) $(CFILE) $(LIBS) Makefile


test: all
	cp p2test/p2test.py ./
	-python3 p2test.py
	rm ./p2test.py


clean:
	rm -f $(ZIPDEST) $(PROGDEST) $(LOGFILE)
