CXXFLAGS = -Wno-reorder -pthread -Wall -g -std=c++11 -I../core/common -I../core/unix
LDFLAGS = -fuse-ld=gold -pthread -static-libstdc++ -static-libgcc -L../ui/linux -L../Onigmo/.libs

pecascape: pecascape.o server.o macro.o template2.o remote.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lpeercast -lrtmp ../Onigmo/.libs/libonigmo.a

.o:.cpp
	$(CXX) $(CXXFLAGS) -c $<

