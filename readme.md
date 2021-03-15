# Othread Obfuscator

Othread is an obfuscation tool based on LLVM, which can perfectly realize the obfuscation of C/C++ code under the Windows platform of x86 architecture.

## 0x01 Principle

We present a confusion idea called Execution trajectory obfuscation which make the program execution trajectory with function as the unit converted from the call execution in a single thread to the jump execution in the mode of repeated switching between multiple threads. Experiment results show that the application of this obfuscation algorithm program can effectively resist the mainstream reverse analysis methods, and at the same time has a low impact on the efficiency of program execution. 

## 0x02 How to use

1. Compile the source code and ".\source\template\templateFileForOthread.cpp" into LLVM intermedia representation
2. Utilize llvm-link to merge all intermedia representation files into one
3. Input the file generated by step2, ".\obfuscator\othread.exe" will output obfuscated intermedia representation
4. Link the file output by step3 to generate an executable file

Usage:othread.exe ..\example\aes\aes.bc ..\example\aes\aes-obf.bc

## 0x03 Example

1. aes		symmetric cryptographic algorithm	Usage:aes.exe
2. rsa		asymmetric cryptographic algorithm	Usage:rsa.exe
3. gzip		Compression algorithm				Usage:gzip.exe ./data/input.combined
4. parser	Lexical analyzer					Usage:parser.exe 2.1.dict
5. twoif	Simulated annealing algorithm		Usage:twoif.exe ./data/test



*Table1 Program execution efficiency before and after obfuscation*

| example | before(s) | after(s) | efficiency |
| ------- | --------- | -------- | ---------- |
| aes     | 0.004     | 0.004    | 95%        |
| rsa     | 0.243     | 0.277    | 87.7%      |
| gzip    | 21.379    | 23.255   | 89.1%      |
| parser  | 0.302     | 0.852    | 35.4%      |
| twoif   | 0.128     | 0.447    | 28.6%      |



Due to the time loss caused by the communication between threads, Othead is mainly suitable for functional programs, the execution efficiency of highly computationally intensive programs (one function called over 10000000 times like simulated annealing algorithm) has decreased significantly



## 0x04 Screenshot

Figure1.1 Function call relationship before and after obfuscation-aes
Figure1.2 Function call relationship before and after obfuscation-rsa
Figure1.3 Function call relationship before and after obfuscation-gzip
Figure1.4 Function call relationship before and after obfuscation-parser
Figure1.5 Function call relationship before and after obfuscation-twoif

Figure2.1 Number of function replacements after obfuscation-aes
Figure2.2 Number of function replacements after obfuscation-rsa
Figure2.3 Number of function replacements after obfuscation-gzip
Figure2.4 Number of function replacements after obfuscation-parser
Figure2.5 Number of function replacements after obfuscation-twoif

Figure3.1 Instructions before and after obfuscation-aes
Figure3.2 Instructions before and after obfuscation-rsa
Figure3.3 Instructions before and after obfuscation-gzip
Figure3.4 Instructions before and after obfuscation-parser
Figure3.5 Instructions before and after obfuscation-twoif

## 0x05 Paper

You can get our paper one week later