# Efficiently detecting use-after-free exploits in multi-threaded applications	

This is my master research project in software reliability and security.

Dangling pointers are a common source of vulnerabilities in widely used C/C++ applications. I designed and implemented a novel technique that detects and mitigates incorrect use of dangling pointers at runtime. The system incurred only 40% average run-time degradation compared to 80% average degradation of previous use-after-free detectors.

Checkout our latest source code at https://github.com/vusec/dangsan/ 

For more information, see the paper "DangSan: Scalable Use-after-free Detection" by Erik van der Kouwe, Vinod Nigade, and Cristiano Giuffrida, presented at the EuroSys 2017 conference.
