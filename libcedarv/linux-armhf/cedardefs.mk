#
# extra includes and libraries for cedarv
# caution: libvecore and libcedarxalloc are shared libraries
#

CEDARINCLUDES = \
        -I$(CEDARDIR) \
        -I$(CEDARDIR)/adapter \
        -I$(CEDARDIR)/adapter/cdxalloc \
	-I$(CEDARDIR)/adapter/avheap \
        -I$(CEDARDIR)/fbm \
        -I$(CEDARDIR)/libcedarv \
        -I$(CEDARDIR)/libvecore \
        -I$(CEDARDIR)/vbv

CEDARLIBS = -L$(CEDARDIR) -lcedarv -L$(CEDARDIR)/libvecore -lvecore
