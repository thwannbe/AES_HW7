CFLAGS=-g -Wall
CC=gcc
LIBS=-lm -lpthread
INCLUDES=-I ./ -I ${BIN_DIR}
BIN_DIR=../bin/
DFTL_DIR=lib/
OBJS=blueftl_user_vdevice.o \
	 blueftl_user_netlink.o \
	 blueftl_ssdmgmt.o \
	 blueftl_util.o \
 	 blueftl_user.o \
	 blueftl_ftl_base.o \
	 blueftl_mapping_dftl_page.o \
	 blueftl_user_ftl_main.o \
	 blueftl_gc_page.o \
	 blueftl_mapping_page.o 
	 

SRCS=$(OBJS.o=.c)

# The variable $@ has the value of the target. In this case $@ = psort
blueftl_user:${OBJS}
	#${CC} ${CFLAGS} ${INCLUDES} -o $@ ${BIN_DIR}blueftl_user.o ${OBJS} ${LIBS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	rm -f blueftl_user
