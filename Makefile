01:
	@ make -C 01_Chunks_of_Bytecode
	@ make -C 01_Chunks_of_Bytecode launch

02:
	@ make -C 02_A_Virtual_Machine
	@ make -C 02_A_Virtual_Machine launch


03:
	@ make -C 03_Scanning_on_Demand
	@ make -C 03_Scanning_on_Demand launch

04:
	@ make -C 04_Compiling_Expressions
	@ make -C 04_Compiling_Expressions launch

05:
	@ make -C 05_Types_of_Values
	@ make -C 05_Types_of_Values launch

06:
	@ make -C 06_Strings
	@ make -C 06_Strings launch

07:
	@ make -C 07_Hash_Tables
	@ make -C 07_Hash_Tables launch

clean:
	@ make -C 01_Chunks_of_Bytecode clean
	@ make -C 02_A_Virtual_Machine clean
	@ make -C 03_Scanning_on_Demand clean
	@ make -C 04_Compiling_Expressions clean
	@ make -C 05_Types_of_Values clean
	@ make -C 06_Strings clean
	@ make -C 07_Hash_Tables clean

.PHONY: 01 02 03 04 05 06 07 clean