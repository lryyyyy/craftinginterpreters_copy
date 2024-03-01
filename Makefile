01:
	@ make -C 01_Chunks_of_Bytecode
	@ make -C 01_Chunks_of_Bytecode launch

02:
	@ make -C 02_A_Virtual_Machine
	@ make -C 02_A_Virtual_Machine launch


03:
	@ make -C 03_Scanning_on_Demand
	@ make -C 03_Scanning_on_Demand launch

clean:
	@ make -C 01_Chunks_of_Bytecode clean
	@ make -C 02_A_Virtual_Machine clean
	@ make -C 03_Scanning_on_Demand clean

.PHONY: 01 02 03 clean