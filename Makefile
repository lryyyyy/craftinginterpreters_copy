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

08:
	@ make -C 08_Global_Variables
	@ make -C 08_Global_Variables launch

09:
	@ make -C 09_Local_Variables
	@ make -C 09_Local_Variables launch

10:
	@ make -C 10_Jumping_Back_and_Forth
	@ make -C 10_Jumping_Back_and_Forth launch

11:
	@ make -C 11_Calls_and_Functions
	@ make -C 11_Calls_and_Functions launch

12:
	@ make -C 12_Closures
	@ make -C 12_Closures launch

clean:
	@ make -C 01_Chunks_of_Bytecode clean
	@ make -C 02_A_Virtual_Machine clean
	@ make -C 03_Scanning_on_Demand clean
	@ make -C 04_Compiling_Expressions clean
	@ make -C 05_Types_of_Values clean
	@ make -C 06_Strings clean
	@ make -C 07_Hash_Tables clean
	@ make -C 08_Global_Variables clean
	@ make -C 09_Local_Variables clean
	@ make -C 10_Jumping_Back_and_Forth clean
	@ make -C 11_Calls_and_Functions clean
	@ make -C 12_Closures clean

.PHONY: 01 02 03 04 05 06 07 08 09 10 11 12 clean
