exe = 		\
	test_tom	\
	test_jerry

all     :       $(exe)

$(exe)  :       %:%.o
	gcc -O2 -g -o $@ $^ -L../src -lhakuna -lpthread -lrt -Wl,-rpath ../src

.c.o	:
	gcc -O2 -g -Wall -D_GNU_SOURCE -o $@ -c $< -I. -I../src -I../include 

clean	:
	rm -f *.o $(exe)

.PHONY : clean all
