# QAic Runtime

QAic Runtime provides the infrastructure to run Machine Learning Workloads on
Qualcomm Cloud AI 100/AIC100 accelerator.  
The user can generate an executable binary of the trained model using QAic Compute compiler and
run inference on the Qualcomm Cloud AI 100.  
The QAic Runtime provides necessary interfaces to allow the user    

*    to identify, query and configure the QAIC 100 Hardware connected to the Host   
*    to write a performant efficient multithreaded user applications.  
 
The repo also bundles example application to demonstrate the usage and capabilities offered by interface.  

## Getting Started
--------------------------------------------------------------------------------------------------------------

### Supported Platforms

QAic Runtime is verified with below listed platform versions.  
Arch: x86 Ubuntu:  Centos:

### Prerequisites

  Build Dependencies
    - CMake 3.24 or higher
      Follow the instructions in [cmake](https://cmake.org/download/) for installation

  Executable binary for QAic 100 Card
     - To generate a executable ML workload runnnable on QAic 100, please follow the instructions at [qualcomm-cloud-ai-100-cc](https://github.com/quic/software-kit-for-qualcomm-cloud-ai-100-cc)

### Building and Testing

   Build
   To build the QAic Runtime, cd to the root directory (where the workspace is cloned) and run the below steps.

```
   aic-runtime$ ./bootstrap.sh
   aic-runtime$ cd build
   aic-runtime$ ../scripts/build.sh
```

   build.sh usage

```
      Usage: build.sh -b [ Release | Debug ] 
     -b to specify the build type 
```

### Run the sample applications.
  Utils directory
  The utils directory contains example applications and source code to demonstrate how to build a QAic Runner
  Application to interact with QAic 100.  
  The below specified directories have further details on how the tools
  work and how they can be used.

*    qaic-util : Tool to list and query the QAIC 100 Cards
    [qaic-util Readme](utils/qaic-util/README)
*    qaic-device-partition : Tool to create a shadow device on a QAIC 100 Card.
    [qaic-device-partition Readme](utils/qaic-device-partition/README)
*    qaic-runner : Example application to demonstrate running inference on QAIC 100.
    [qaic-runner Readme](utils/qaic-runner/README.md)

  Sample Command:

```
     aic-runtime/build$ ./utils/qaic-util/qaic-util -q
     aic-runtime/build$ ./utils/qaic-util/qaic-runner -d 0 -t <path_to_executable>
```

## Repo Structure
--------------------------------------------------------------------------------------------------------------  

This tree demonstrates the layout of open-runtime repo.

```
├── inc
│   └── qaic-openrt-api-hpp <The public interface headers of QAic-runtime>
├── lib                     <Internal library components of QAic-runtime>
├── README.md
├── scripts
│   ├── build.sh
│   └── format.sh
├── test                     <gtest based UnitTest binaries>
│   └── qaic-openrt-api-test
├── thirdparty-modules
└── utils                     <Example Application and Tools>
    ├── qaic-device-partition  < Example Application to create partition devices on a physical device >
    ├── qaic-runner            < Example Inference Application >
    └── qaic-util              < Device query tool >
```

## License
QAic Runtime is licensed under the terms in the [LICENSE](LICENSE) file.
