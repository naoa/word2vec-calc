CC = gcc
#The -Ofast might not work with older versions of gcc; in that case, use -O2
CFLAGS = -lm -pthread -Ofast -march=native -Wall -funroll-loops -Wno-unused-result \
         -lstdc++ -lre2 -lgflags `icu-config  --cppflags --ldflags`
all: word2vec-calc

word2vec-calc : word2vec-calc.cpp
	$(CC) word2vec-calc.cpp -o word2vec-calc $(CFLAGS)

clean:
	rm -rf word2vec-calc
