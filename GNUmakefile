
include ../../GDALmake.opt

OBJ	=	RasterJSON.o $(AIGOBJ)

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS) -I./jsonpull/src

default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f $(OBJ)

$(OBJ):	RasterJSON.h

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))

