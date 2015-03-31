CC =		mpicc
CCFLAGS =	-g -O -I headers
DEBUG = 	
PRINT = 
TEST_SIZE =	
TIMING =
PERF_EVAL =	
STORAGE =
BLOCK_SIZE = 
INPUT_DATA = 
FLUSH_RATE = 
LINK =		mpicc
LINKFLAGS =	-g -O -I headers

all: squirrelDB

squirrelDB:	squirrelDB.o masterStorage.o slaveStorage.o MurmurHash3.o requests.o
	$(LINK) $(LINKFLAGS) masterStorage.o slaveStorage.o requests.o squirrelDB.o MurmurHash3.o -o squirrelDB

test_1:	test_1.o squirrelDBClient.o
	$(LINK) $(LINKFLAGS) test_1.o squirrelDBClient.o -o test_1
	make clean_flush_files
	make run DEBUG=-DDEBUG PRINT=-DPRINT

test_2:	test_2.o squirrelDBClient.o
	$(LINK) $(LINKFLAGS) test_2.o squirrelDBClient.o -o test_2
	make clean_flush_files
	make run DEBUG=-DDEBUG PRINT=-DPRINT

test_3:	test_3.o squirrelDBClient.o
	$(LINK) $(LINKFLAGS) test_3.o squirrelDBClient.o -o test_3
	make clean_flush_files
	cp ../flush_files/*.sqdb ../
	ssh eaframe@cslvm27.csc.calpoly.edu 'cp ../SquirrelDB/flush_files/*.sqdb ../SquirrelDB/'
	ssh eaframe@cslvm28.csc.calpoly.edu 'cp ../SquirrelDB/flush_files/*.sqdb ../SquirrelDB/'
	ssh eaframe@cslvm29.csc.calpoly.edu 'cp ../SquirrelDB/flush_files/*.sqdb ../SquirrelDB/'
	ssh eaframe@cslvm30.csc.calpoly.edu 'cp ../SquirrelDB/flush_files/*.sqdb ../SquirrelDB/'
	make run DEBUG=-DDEBUG TEST_SIZE=-DSMALL_TEST

shell:	squirrelDBShell.o squirrelDBClient.o
	$(LINK) $(LINKFLAGS) squirrelDBShell.o squirrelDBClient.o -o squirrelDBShell

perf_eval: perf_eval.o squirrelDBClient.o
	$(LINK) $(LINKFLAGS) perf_eval.o squirrelDBClient.o -o perf_eval

%.o:%.c
	$(CC) $(CCFLAGS) $(DEBUG) $(PRINT) $(TEST_SIZE) $(TIMING) $(PERF_EVAL) $(STORAGE) $(BLOCK_SIZE) $(INPUT_DATA) $(FLUSH_RATE) -c $<


run:	squirrelDB
	make clean
	make squirrelDB
	cp squirrelDB ../
	scp squirrelDB eaframe@cslvm27.csc.calpoly.edu:../SquirrelDB
	scp squirrelDB eaframe@cslvm28.csc.calpoly.edu:../SquirrelDB
	scp squirrelDB eaframe@cslvm29.csc.calpoly.edu:../SquirrelDB
	scp squirrelDB eaframe@cslvm30.csc.calpoly.edu:../SquirrelDB
	cd /home/SquirrelDB/ && \
	/home/SquirrelDB/.openmpi/bin/mpirun -n 5 -mca plm_rsh_no_tree_spawn 1 --host cslvm26.csc.calpoly.edu,cslvm27.csc.calpoly.edu,cslvm28.csc.calpoly.edu,cslvm29.csc.calpoly.edu,cslvm30.csc.calpoly.edu squirrelDB


commit:
	git commit -am "$(MSG)"

pull:
	git pull origin squirrelDB

push:
	git push origin squirrelDB

sync:
	git commit -am "$(MSG)"
	git push origin squirrelDB
clean:
	rm -f *.o squirrelDB

clean_tests:
	rm -f test_1 test_1.o test_2 test_2.o test_3 test_3.o squirrelDBClient.o

clean_flush_files:
	rm -f ../*.sqdb
	ssh eaframe@cslvm27.csc.calpoly.edu 'rm -f ../SquirrelDB/*.sqdb'
	ssh eaframe@cslvm28.csc.calpoly.edu 'rm -f ../SquirrelDB/*.sqdb'
	ssh eaframe@cslvm29.csc.calpoly.edu 'rm -f ../SquirrelDB/*.sqdb'
	ssh eaframe@cslvm30.csc.calpoly.edu 'rm -f ../SquirrelDB/*.sqdb'
