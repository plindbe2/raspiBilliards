#*****************************************************
#  Particles makefile
#
#  Phil Lindberg
#
#*****************************************************

OBJS = 

EXENAME = billiards

COMMONSRC=esShader.c    \
          esTransform.c \
          esShapes.c    \
          esUtil.c

vpath %.c ./src
vpath %.h ./include

CC = gcc
CFLAGS=-Wall
DEFINES=-DRPI_NO_X
INCDIR=-I../include -I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux
LIBS=-lGLESv2 -lEGL -lm -lbcm_host -L$(SDKSTAGE)/opt/vc/lib -lpng

default: all

.PHONY: all
all: $(EXENAME)

.PHONY: clean
clean:
	-rm *.o $(EXENAME)

$(EXENAME) : billiards.o esShader.o esShapes.o esTransform.o esUtil.o glesTools.o glesVMath.o
	$(CC) ${CFLAGS} ${DEFINES} $^ -o ./$@ ${LIBS}
glesTools.o : glesTools.c glesTools.h
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
glesVMath.o : glesVMath.c glesVMath.h
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
billiards.o : billiards.c esShader.o esShapes.o esTransform.o esUtil.o esUtil.h
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
esShader.o : esShader.c
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
esShapes.o : esShapes.c
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
esTransform.o : esTransform.c
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
esUtil.o : esUtil.c
	$(CC) ${CFLAGS} ${DEFINES} -c $< -o ./$@ ${INCDIR} ${LIBS}
