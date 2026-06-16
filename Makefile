SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
COPT = -lusb-1.0
all: $(OBJ)
	@echo $(SRC) 
	gcc -o out $^ $(COPT)

%.o : %.c
	gcc -o $@ -c $<

clean:
	rm -rf *.o
