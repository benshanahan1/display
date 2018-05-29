.PHONY: all
all: demo

demo:
	gcc demo.c -o demo -I../src -L../ -Wl,-rpath=../ -ldisplay

clean: 
	rm -f demo testOutput.txt