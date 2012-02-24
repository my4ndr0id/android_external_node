all : memleak

memleak : memleak.o main.o
	g++ -g -rdynamic -fPIC -m32 -lpthread $^ -o $@

%.o : %.cc memleak.h
	g++ -g -rdynamic -m32 -c $<

clean :
	rm *.o
