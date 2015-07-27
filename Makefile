# FlyCapture2Test makefile
# To compile the debug verison need to overwrite CXXFLAGS variable to include -ggdb

CC = g++
OUTPUTNAME = lfapp${D}
INCLUDE = -I../../include -I/usr/include/flycapture
LIBS = -L../lib -lflycapture${D}
CFLAGS = -g

OUTDIR = ../laserfence-bin

OBJS = lfapp.o lfcam.o

${OUTPUTNAME}: ${OBJS}
	${CC} -o ${OUTPUTNAME} ${OBJS} ${LIBS} ${COMMON_LIBS}
	mv ${OUTPUTNAME} ${OUTDIR}
	ctags -R .

%.o: %.cpp
	${CC} ${CFLAGS} ${INCLUDE} -c $*.cpp
	
clean_obj:
	rm -f ${OBJS}
	@echo "all cleaned up!"

clean:
	rm -f ${OUTDIR}/${OUTPUTNAME} ${OBJS}
	@echo "all cleaned up!"
