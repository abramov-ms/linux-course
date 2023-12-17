obj-m += mmaneg.o

all: mmaneg

phonebook:
	make -C ../ M=$(PWD) modules

clean:
	make -C ../ M=$(PWD) clean