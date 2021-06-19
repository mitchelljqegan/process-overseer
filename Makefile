CC = gcc 
CFLAGS = -pthread -Wall
all: overseer controller

overseer: overseer.c overseer_functions.c

controller: controller.c controller_functions.c

clean:
	rm -f overseer controller
 
.PHONY: all clean
