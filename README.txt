This code is a simple application which mimics a machine learning application.
This application loads Cifar10 into enclave threads which access it independently --- Asynchronously.

This code does nothing more than prove that Cifar10 under SGX is susceptible to side channels, and SGX threads are susceptible to OS scheduling manipulation.

Originally based off the Asynchronous "Power Transition Sample" from the SGX-SDK Samples directory:
    https://github.com/intel/linux-sgx/tree/master/SampleCode/PowerTransition

Cifar10 Reader from 
    https://baptiste-wicht.com/posts/2017/10/deep-learning-library-10-fast-neural-network-library.html
    
    Please note you need to download the dataset. Use the download script at https://github.com/wichtounet/cifar-10/blob/ad30996c9b853e9805166beb0f929fc747ab3148/download_cifar10.sh, as decscribed in the source: https://github.com/wichtounet/cifar-10/tree/ad30996c9b853e9805166beb0f929fc747ab3148

------------------------------------
How to Build/Execute the Sample Code
------------------------------------
1. Install Intel(R) Software Guard Extensions (Intel(R) SGX) SDK for Linux* OS
2. Build the project with the prepared Makefile:
    a. Hardware Mode, Debug build:
        $ make
    b. Hardware Mode, Pre-release build:
        $ make SGX_PRERELEASE=1 SGX_DEBUG=0
    c. Hardware Mode, Release build:
        $ make SGX_DEBUG=0
    d. Simulation Mode, Debug build:
        $ make SGX_MODE=SIM
    e. Simulation Mode, Pre-release build:
        $ make SGX_MODE=SIM SGX_PRERELEASE=1 SGX_DEBUG=0
    f. Simulation Mode, Release build:
        $ make SGX_MODE=SIM SGX_DEBUG=0
3. Execute the binary directly:
    $ ./app
4. Remember to "make clean" before switching build mode
