qaic-runner is a sample application to run inference on Qualcomm AIC 100 cards

# Getting Started
----------------------------------------------------------------------------------------------

## Pre-requisites:
1. Qualcomm AIC 100 card connected to the Host.
2. QAIC 100 card installed with Platform SDK compatible with QAic Runtime.
3. qaic-runner executable binary.
4. Program binary of an executable ML workload runnnable on QAic 100.

## Usage:
Please find below the location of the binary from root directory of QAic Runtime  
```
$WS> ./build/utils/qaic-runner/qaic-runner -h  
```  

```
Usage: qaic-runner [options]  
  -d, --aic-device-id <id>            AIC device ID, default 0  

  -t, --test-data <path>                Location of program binaries  

  -i, --input-file <path>               Input filename from which to load input data.  
                                  Specify multiple times for each input file.  
                                  If no -i is given, random input will be generated  

  -o, --output-file <path>              Output filename from which to compare output for validation.  
                                  Specify multiple times for each output file.  
                                  If no -o is given, no validation of output will be done.  

  -n, --num-iter <num>                 Number of iterations, default 1  

  --write-output-start-iter <num>      Write outputs start iteration, default 0  

  --write-output-num-samples <num>     Number of outputs to write, default 1  

  --write-output-dir <path>             Location to save output files, dir should exist and be writable, default '.'  

  -v, --verbose                   Verbose log from program  

  -h, --help                      help  

```

## Run inferences:
----------------------------------------------------------------------------------------------  
## 1.1 Run default number of inferences for a ML workload
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile

## 1.2 Run inference on specific AIC100 card
 The AIC device id can be identified using qaic-util tool. Sample input for QID 1:
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -d 1

## 1.3 Run inference with specific input
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -i inputfile

## 1.4 Validate output of inference with provided outputfile
 The "-o" option is to validate output of the inference runs. If output of inference doesnot match
 with provided output, validation failure will be notified to user. In case, number of inferences
 is more than 1, output validation is done for all the inference iterations.
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -i inputfile -o outputfile

## 1.5 Run specific number of inferences
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -n 5

## 1.6 Run inferences and dump output of specific inference runs to provided location
 Below sample command dumps the output of inference iteration 2, 3 and 4 to the current directory.
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -n 10 --write-output-start-iter 2 --write-output-num-samples 3 --write-output-dir .

## 1.7 Run inference with verbose logs
 Sample command to print debug level verbose output logs.
> ### sudo ./qaic-runner -t MLWorkloadExecutableBinFile -vvv
