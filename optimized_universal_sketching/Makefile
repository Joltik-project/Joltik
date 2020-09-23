CC=gcc -g
#GADGETLIB=GadgetReader
IFLAG=
#LFLAG= $(GADGETLIB)/gadgetreader.o
# LINKER= -lm -lpcap
LINKER= -lm
PROFILER=
HEADER = univmon.h helper/input.h

# all: testSketch realfreq
all: testSketch estiSketch

# testSketch
testSketch: univmon.o heap.o 
	$(CC) -o bin/testSketch bin/univmon.o bin/heap.o $(LINKER) $(PROFILER)	

heap.o:
	$(CC) -c -Wall helper/heap.c -o bin/heap.o$(PROFILER)	

univmon.o: $(HEADER)
	$(CC) -c -Wall univmon.c -o bin/univmon.o$(IFLAG) $(PROFILER)

estiSketch: univmon_offline.o heap.o 
	$(CC) -o bin/estiSketch bin/univmon_offline.o bin/heap.o $(LINKER) $(PROFILER)	

univmon_offline.o: $(HEADER)
	$(CC) -c -Wall univmon_offline.c -o bin/univmon_offline.o$(IFLAG) $(PROFILER)

clean:
	rm -f bin/testSketch
	rm -f bin/estiSketch
	rm -f bin/*.o
	rm -f bin/*.txt
	rm -f bin/offline_result/*
	# rm -f bin/counters*
