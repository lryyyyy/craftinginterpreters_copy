01:
	@ make -C 01_Chunks_of_Bytecode
	@ make -C 01_Chunks_of_Bytecode launch

clean:
	@ make -C 01_Chunks_of_Bytecode clean

.PHONY: 01 clean