CXX=g++
OFILES=project2.cpp
.SUFFIXES: .o .cpp

wordPuzzle: $(OFILES)
	$(CXX) $(OFILES)
	./a.out

clean:
	-rm -f *.o *.cpp~

