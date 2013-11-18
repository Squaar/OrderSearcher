Matt Dumford - mdumford
mdumfo2@uic.edu

CS 385 - Homework 3 - Searching for Order Among Chaos Using Threads

OrderSearcher.c
	./orderSearcher file nThreads

	file: the file that orderSearcher will read and compute statistics for
	nThreads: the number of threads that orderSearcher will use

OrderSearcher will read in a file and split it up evenly among nThread threads. 
These threads compute the best (minimum) values for the following statistics on 
their portions of the file: 

	- The range
	- Maximum absolute value of change from one value to the next
	- The sum of ( absolute value of ( change from one value to the next ))
	- Standard deviation
	- Standard deviation of the change between each element

There are no known errors, but the range and maximum absolute value of change from one value
to the next almost always return 255 and 0 respectively. This is accurate though because almost
any file will have a 0 and a 255 in it and two of the same number in a row somewhere.