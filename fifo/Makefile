obj-m += fifo.o

all: fifo

fifo:
	make -C ../ M=$(PWD) modules

clean:
	make -C ../ M=$(PWD) clean
