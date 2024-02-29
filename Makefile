01:
	@ make -C 01_Chunks_of_Bytecode
	@ make -C 01_Chunks_of_Bytecode launch

02:
	@ make -C 02_A_Virtual_Machine
	@ make -C 02_A_Virtual_Machine launch

clean:
	@ make -C 01_Chunks_of_Bytecode clean
	@ make -C 02_A_Virtual_Machine clean

.PHONY: 01 02 clean