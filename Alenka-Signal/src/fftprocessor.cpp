#include "../include/AlenkaSignal/fftprocessor.h"
#include "filterprocessor.cpp"
#include "../include/AlenkaSignal/openclcontext.h"
#include "../include/AlenkaSignal/openclprogram.h"

#include <clFFT.h>

#ifndef WIN_BUILD
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif
#include <fasttransforms.h>
#ifndef WIN_BUILD
#pragma GCC diagnostic pop
#endif

#include <cmath>
#include <type_traits>
//TODO: DELETE
#include <iostream>

#include <detailedexception.h>
using namespace std;

namespace AlenkaSignal {
FftProcessor::FftProcessor( OpenCLContext *context, int parallelQueues)
    : parallelQueues(parallelQueues), context(context) {
  cl_int err;
  queue = clCreateCommandQueue(context->getCLContext(), context->getCLDevice(), 0, &err);
  checkClErrorCode(err, "clCreateCommandQueue");
}

FftProcessor::~FftProcessor() {
  cl_int err;
  deleteBuffers();
  deletePlans();

  //TODO: there are parallel queues in signalproc, and one in canvas if no glsharing
  //cache in canvas is scaled by queue number
  //solve interaction with other queues, mby use queues from other component
  if (queue) {
    err = clReleaseCommandQueue(queue);
    checkClErrorCode(err, "clReleaseCommandQueue()");
  }
}

void FftProcessor::createBuffers() {
  cl_int err;
  inBuf = clCreateBuffer(context->getCLContext(), CL_MEM_READ_WRITE, batchSize * fftSize * sizeof(float),
    NULL, &err);

  checkClErrorCode(err, "clCreateBuffer");

  outBuf = clCreateBuffer(context->getCLContext(), CL_MEM_READ_WRITE, batchSize * (fftSize / 2 + 1)
    * sizeof(std::complex<float>), NULL, &err);

  checkClErrorCode(err, "clCreateBuffer");

  err = clFinish(queue);
}

void FftProcessor::deleteBuffers() {
  cl_int err;
  //TODO: error, add this to cleanup in docker widget?
  if (inBuf) {
    err = clReleaseMemObject(inBuf);
    checkClErrorCode(err, "clReleaseMemObject");
  }

  if (outBuf) {
    err = clReleaseMemObject(outBuf);
    checkClErrorCode(err, "clReleaseMemObject");
  }
}

void FftProcessor::createPlans() {
  cl_int err;
  clfftStatus errFFT;

  clfftPrecision precision = CLFFT_SINGLE;

  errFFT = clfftCreateDefaultPlan(&fftPlan, context->getCLContext(),
    CLFFT_1D, &fftSize);
  checkClfftErrorCode(errFFT, "clfftCreateDefaultPlan()");
  errFFT = clfftSetPlanPrecision(fftPlan, precision);
  checkClfftErrorCode(errFFT, "clfftSetPlanPrecision()");
  errFFT =
    clfftSetLayout(fftPlan, CLFFT_REAL, CLFFT_HERMITIAN_INTERLEAVED);
  checkClfftErrorCode(errFFT, "clfftSetLayout()");
  errFFT = clfftSetResultLocation(fftPlan, CLFFT_OUTOFPLACE);
  checkClfftErrorCode(errFFT, "clfftSetResultLocation()");
}

void FftProcessor::deletePlans() {
  clfftStatus errFFT;
  if (fftPlan) {
    errFFT = clfftDestroyPlan(&fftPlan);
    checkClfftErrorCode(errFFT, "clfftDestroyPlan()");
  }
}

void FftProcessor::updateFft() {
  clfftStatus errFFT;
  cl_int err;

  deleteBuffers();

  errFFT = clfftSetPlanBatchSize(fftPlan, batchSize);
  checkClfftErrorCode(errFFT, "clfftSetPlanBatchSize()");
  errFFT =
    clfftSetPlanDistance(fftPlan, fftSize, (fftSize / 2) + 1);
  checkClfftErrorCode(errFFT, "clfftSetPlanDistance()");

  createBuffers();
}

std::vector<std::complex<float>> FftProcessor::process(const std::vector<float>& input,
  OpenCLContext *gcontext, int batchSize, int fftSize) {
  if (this->batchSize != batchSize || this->fftSize != fftSize) {
    this->batchSize = batchSize;
    this->fftSize = static_cast<size_t>(fftSize);

    deletePlans();
    createPlans();
    updateFft();
  }

  cl_int err;
  clfftStatus errFFT;
  context = gcontext;

  std::vector<std::complex<float>> result(batchSize * (fftSize / 2 + 1), 0);
  int ref;

  err = clEnqueueWriteBuffer(queue, inBuf, CL_TRUE, 0,
    batchSize * fftSize * sizeof(float), input.data(), 0, NULL, NULL);

  checkClErrorCode(err, "clEnqueueWriteBuffer");

  errFFT =
      clfftEnqueueTransform(fftPlan, CLFFT_FORWARD, 1, &queue, 0, nullptr,
                            nullptr, &inBuf, &outBuf, nullptr);
  checkClfftErrorCode(errFFT, "clfftEnqueueTransform");

  /* Wait for calculations to be finished. */
  err = clFinish(queue);

  checkClErrorCode(err, "clFinish");

  /* Fetch results of calculations. */
  err = clEnqueueReadBuffer(queue, outBuf, CL_TRUE, 0, batchSize * (fftSize / 2 + 1) *
    sizeof(std::complex<float>), result.data(), 0, NULL, NULL);

  checkClErrorCode(err, "clEnqueueReadBuffer");

  err = clFinish(queue);

  checkClErrorCode(err, "clFinish");

  return result;
}

} // namespace AlenkaSignal
