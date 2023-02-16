// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QAicRunner.h"

using namespace qaicrunner;

// clang-format off
static void usage() {
  printf(
       "Usage: qaic-runner [options]\n"
         "  -d, --aic-device-id <id>              AIC device ID, default %d\n"
         "  -t, --test-data <path>                Location of program binaries\n"
         "  -i, --input-file <path>               Input filename from which to load input data.\n"
         "                                        Specify multiple times for each input file.\n"
         "                                        If no -i is given, random input will be generated\n"
         "  -o, --output-file <path>              Output filename from which to compare output for validation.\n"
         "                                        Specify multiple times for each output file.\n"
         "                                        If no -o is given, no validation of output will be done.\n"
         "  -n, --num-iter <num>                  Number of iterations, default %d\n"
         "  --write-output-start-iter <num>       Write outputs start iteration, default %d\n"
         "  --write-output-num-samples <num>      Number of outputs to write, default %d\n"
         "  --write-output-dir <path>             Location to save output files, dir should exist and be writable, default '%s'\n"
         "  -v, --verbose                         Verbose log from program\n"
         "  -h, --help                            help\n",
         qidDefault, // --aic-device-id
         numInferencesDefault, // --num-iter
         startIterationDefault, // --write-output-start-iter
         numSamplesDefault, // --write-output-num-samples
         outputDirDefault.c_str() // --write-output-out-dir
         );
}
// clang-format on

int main(int argc, char **argv) {
  QStatus status;
  QAicRunnerExample runner;
  uint32_t numInferences = numInferencesDefault;
  uint32_t qid = qidDefault;
  uint32_t verbose = 0;

  // Long Option Structure
  // const char *name;
  //    int has_arg;
  //    int *flag;
  //    int val;
  struct option long_options[] = {
      {"aic-device-id", required_argument, 0, 'd'},
      {"test-data", required_argument, 0, 't'},
      {"input-file", required_argument, 0, 'i'},
      {"output-file", required_argument, 0, 'o'},
      {"num-iter", required_argument, 0, 'n'},
      {"verbose", no_argument, 0, 'v'},
      {"write-output-start-iter", required_argument, 0, 1},
      {"write-output-num-samples", required_argument, 0, 2},
      {"write-output-dir", required_argument, 0, 3},
      {0, 0, 0, 0}};

  int option_index = 0;
  int opt;

  while ((opt = getopt_long_only(argc, argv, "d:t:i:o:n:vh", long_options,
                                 &option_index)) != -1) {
    switch (opt) {
    case 1: // write-output-start-iter
      if (std::atoi(optarg) < 0) {
        std::cerr << "Set non-negative value for write-output-start-iter"
                  << std::endl;
        exit(1);
      }
      runner.setWriteOutputStartIteration(std::atoi(optarg));
      break;
    case 2: // write-output-num-samples
      if (std::atoi(optarg) <= 0) {
        std::cerr << "Set positive value for write-output-num-samples"
                  << std::endl;
        exit(1);
      }
      runner.setWriteOutputNumSamples(std::atoi(optarg));
      break;
    case 3: // write-output-dir
      if (!runner.setWriteOutputDir(optarg)) {
        std::cerr << "Invalid output dir: " << optarg << std::endl;
        usage();
        exit(1);
      }
      break;
    case 'd': // aic-device-id
      if (std::atoi(optarg) < 0) {
        std::cerr << "Set a valid aic-device-id" << std::endl;
        exit(1);
      }
      qid = std::atoi(optarg);
      break;
    case 't': // test-data
      runner.setTestBasePath(std::string(optarg));
      break;
    case 'i': // input-file
      runner.addToInputFileList(std::string(optarg));
      break;
    case 'o': // output-file
      runner.addToOutputFileList(std::string(optarg));
      break;
    case 'n': // num-iter
      if (std::atoi(optarg) <= 0) {
        std::cerr << "Set a positive value for num-iter" << std::endl;
        exit(1);
      }
      numInferences = std::atoi(optarg);
      break;
    case 'v': // verbose
      verbose++;
      break;
    case 'h':
    case '?':
      usage();
      exit(0);
    default:
      usage();
      exit(1);
    }
  }

  runner.setQid(qid);
  runner.setVerbosity(verbose);
  runner.setNumInferences(numInferences);

  if (runner.checkUserParam()) {
    std::cerr << "Failed to validate arguments" << std::endl;
    usage();
    exit(1);
  }

  try {
    status = runner.init();

    if (status != QS_SUCCESS) {
      std::cerr << "Failed to initialize QAIC runner" << std::endl;
      return 1;
    }

    status = runner.run();

    if (status != QS_SUCCESS) {
      std::cerr << "Failed to run QAIC runner" << std::endl;
      return 1;
    }

    uint64_t totalInferencesCompleted = 0;
    double infRate = 0;
    uint64_t runtimeUs = 0;
    uint32_t batchSize = 1;

    runner.getLastRunStats(totalInferencesCompleted, infRate, runtimeUs,
                           batchSize);

    std::cout << " ---- Stats ----" << std::endl;
    std::cout << "InferenceCnt " << totalInferencesCompleted
              << " TotalDuration " << std::fixed << runtimeUs << "us"
              << " BatchSize " << batchSize << " Inf/Sec "
              << std::setprecision(3) << infRate << std::endl;

  } catch (const qaic::openrt::CoreExceptionInit &e) {
    std::cerr << "Caught Initialization Exception" << e.what() << std::endl;
    return 1;
  } catch (const qaic::openrt::CoreExceptionRuntime &e) {
    std::cerr << "Caught Runtime Exception " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
