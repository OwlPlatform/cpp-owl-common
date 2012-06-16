SRCDIR=src
INCDIR=include
LIBDIR=lib
INSTALL_DIR=/usr/local/lib
CONFIG_DIR=/usr/share/grail3

define sources
	$(filter %.cpp, $^)
endef
define objects
	$(filter %.o, $^)
endef

#Variables for implicit compilation rules
CXX=g++
#CXXFLAGS=--debug -std=c++0x -Wall -Wextra -Wno-sign-compare -I./$(INCDIR) -L./$(LIBDIR)
CXXFLAGS=-O3 -std=c++0x -Wall -Wextra -Wno-sign-compare -I./$(INCDIR) -L./$(LIBDIR)

dummy:=$(shell mkdir -p $(LIBDIR))

#Rule to make an object file from a cpp file
$(LIBDIR)/%.o : $(SRCDIR)/%.cpp $(INCDIR)/%.hpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

#Same same rul but with a file with .h instead of .hpp
$(LIBDIR)/%.o : $(SRCDIR)/%.cpp $(INCDIR)/%.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(LIBDIR)/libowl.a: $(LIBDIR)/sensor_aggregator_protocol.o $(LIBDIR)/aggregator_solver_protocol.o $(LIBDIR)/world_model_protocol.o $(LIBDIR)/netbuffer.o $(LIBDIR)/sample_data.o $(LIBDIR)/simple_sockets.o
	ar crs $@ $^

install:
	cp $(LIBDIR)/* $(INSTALL_DIR)

clean:
	-rm $(LIBDIR)/*.o
	-rm $(LIBDIR)/*.a

.SILENT: clean
