obj-m += phonebook.o

all: phonebook

phonebook:
	make -C ../../ M=$(PWD) modules

clean:
	make -C ../../ M=$(PWD) clean
