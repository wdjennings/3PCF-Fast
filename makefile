# Run make 2> >(python filter-noisy-assembler-warnings.py)
# To hide the annoying warnings on MacBook

# Compiler
CXX = g++

# Library flags
fftwf = -lfftw3f -lfftw3f_threads
fftwd = -lfftw3 -lfftw3_threads
omp = -fopenmp -D_OMPTHREAD_
gsl = -lgsl -lgslcblas

# Other flags
CFLAGS = -Wall -Wno-unused-variable $(omp) $(FFTW)
LFLAGS = -Wall -Wno-unused-variable $(omp) -lm $(FFTW) $(gsl)

# Helper tool objects
OBJS = cpp_tools/timer.o \
	cpp_tools/point.o \
	cpp_tools/data_vectors.o \
	cpp_tools/string_ext.o \
	cpp_tools/dir_ext.o \
	cpp_tools/loop_data.o	   

driver: driver.o ${OBJS} bins.o corr3.o globals.hpp
	${CXX} -o driver $^ $(LFLAGS)

driver.o: driver.cc
	${CXX} -c -o $@ $< ${CFLAGS}

corr3.o: corr3.cc corr3.hpp
	${CXX} -c -o $@ $< ${CFLAGS}

bins.o: bins.cc bins.hpp
	${CXX} -c -o $@ $< ${CFLAGS}

# Make object files from .cc files
%.o: %.cc %.hpp
	${CXX} -c -o $@ $< ${CFLAGS}

clean:
	rm driver *.o cpp_tools/*.o $(OBJS) 2>/dev/null || true

