.POSIX:

.PHONY: all clean

# Type "make" to download Cereal and compile the program:
all: classy-alarm-clock

classy-alarm-clock: Classy\ Alarm\ Clock/main.cpp
	# If cereal does not exist, then run "git clone ..." to get it.
	test -e cereal || git clone https://github.com/USCiLab/cereal.git cereal
	g++ -Wall -std=c++11 -Icereal/include Classy\ Alarm\ Clock/main.cpp -lncurses -o classy-alarm-clock

clean:
	rm classy-alarm-clock
