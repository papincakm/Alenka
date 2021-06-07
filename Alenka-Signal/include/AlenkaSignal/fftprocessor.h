#ifndef ALENKASIGNAL_FFTPROCESSOR_H
#define ALENKASIGNAL_FFTPROCESSOR_H

#ifdef __APPLE__
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl_gl.h>
#endif

#include <cassert>
#include <vector>
#include <memory>
#include <complex>

typedef size_t clfftPlanHandle;

namespace AlenkaSignal {

class OpenCLContext;

/**
 * @brief This class handles filtering of data blocks.
 */
class FftProcessor {
public:
  FftProcessor(OpenCLContext *context, int parallelQueues);
  ~FftProcessor();

  std::vector<std::complex<float>> process(const std::vector<float>& input,
    OpenCLContext *gcontext, int batchSize, int fftSize);
private:
  int parallelQueues = 0;
  int batchSize = 0;
  size_t fftSize = 0;

  cl_command_queue queue = nullptr;
  cl_mem inBuf = nullptr;
  cl_mem outBuf = nullptr;
  clfftPlanHandle fftPlan = 0;

  std::vector<cl_command_queue> commandQueues;
  OpenCLContext*context;
  //std::unique_ptr<AlenkaSignal::OpenCLContext> context;

  void updateFft();
  void deleteBuffers();
  void deletePlans();
  void createBuffers();
  void createPlans();
};

} // namespace AlenkaSignal

#endif // ALENKASIGNAL_FFTPROCESSOR_H
