bin=mytestStatus
src=testStatus.c
$(bin):$(src)
	@gcc -o $@ $^
	@echo "complier $(src) to $(bin) .." 

.PHONY:clean
clean:
	@rm -rf $@
	@echo "delete complete!"









#  my.exe:code.o
#			gcc code.o -o my.exe 
#code.o:code.s
#	gcc -c code.s -o code.o
#code.s:code.i
#	gcc -S code.i -o code.s
#code.i:code.c
#	gcc -E code.c -o code.i
#.PHONY:clean
#	rm -rf my.exe
