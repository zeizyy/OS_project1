CXX=g++
OFILES=project1.cpp
.SUFFIXES: .o .cpp

wordPuzzle: $(OFILES)
	$(CXX) -o shell $(OFILES)
	./shell

clean:
	-rm -f *.o *.cpp~