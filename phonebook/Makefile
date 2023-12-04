obj-m += phonebook.o

all: phonebook test

phonebook:
	make -C ../ M=$(PWD) modules

test:
	clang phonebook_test.c -static -o phonebook_test

clean:
	make -C ../ M=$(PWD) clean
	rm phonebook_test
