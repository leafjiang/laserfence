# FlyCapture2Test makefile
# To compile the debug verison need to overwrite CXXFLAGS variable to include -ggdb

CC = g++
OUTPUTNAME = lfapp${D}
INCLUDE = -I../../include -I/usr/include/flycapture
LIBS = -L../lib -lflycapture${D} -lboost_program_options
GIT_VERSION = $(shell git describe --abbrev=4 --dirty --always --tags)
CFLAGS = -g -DVERSION=\"$(GIT_VERSION)\"

OUTDIR = ../../laserfence-bin

OBJS = lfcamapp.o lfcam.o lfalg.o

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
	rm -f gitversion.c
	@echo "all cleaned up!"



