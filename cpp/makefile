CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -DNDEBUG

# list or numbers
ORDER = $(shell cat order.txt)

N_THREADS = 16

all: $(ORDER)
%:
	$(CXX) $(CXXFLAGS) face_cube.cpp -DTEST_FACE_BEAM_SEARCH -DORDER=$@ -DDEPTH=7 -DN_THREADS=$(N_THREADS) -o bin/face_solve_$@_7_normal
	$(CXX) $(CXXFLAGS) face_cube.cpp -DTEST_FACE_BEAM_SEARCH -DORDER=$@ -DDEPTH=8 -DN_THREADS=$(N_THREADS) -o bin/face_solve_$@_8_normal
	$(CXX) $(CXXFLAGS) face_cube.cpp -DTEST_FACE_BEAM_SEARCH -DORDER=$@ -DDEPTH=7 -DN_THREADS=$(N_THREADS) -o bin/face_solve_$@_7_rainbow -DRAINBOW
	$(CXX) $(CXXFLAGS) face_cube.cpp -DTEST_FACE_BEAM_SEARCH -DORDER=$@ -DDEPTH=8 -DN_THREADS=$(N_THREADS) -o bin/face_solve_$@_8_rainbow -DRAINBOW