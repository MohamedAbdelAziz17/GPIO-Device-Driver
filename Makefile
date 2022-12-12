obj-m += main.o

all:
	make -C /lib/modules/5.15.81-v7+/build M=$(PWD) modules

clean:
	make -C /lib/modules/5.15.81-v7+/build M=$(PWD) clean
	