bin=mycode
src=code.c
bin1=mytest
src1=test.c
$(bin1):$(src1)

	@gcc -o $@ $^
	@echo "complier $(src1) to $(bin1) .." 
$(bin):$(src)
	@gcc -o $@ $^
	@echo "complier $(src) to $(bin) .." 

.PHONY:clean
clean:
	@rm -rf mycode
	@rm -rf mytest 
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
