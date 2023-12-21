obj-m += mmaneg.o

all: mmaneg

mmaneg:
	make -C ../ M=$(PWD) modules

clean:
	make -C ../ M=$(PWD) clean