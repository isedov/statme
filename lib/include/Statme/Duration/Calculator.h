#pragma once

#include <memory>
#include <stdint.h>
#include <string>

#include <Statme/Duration/Measurement.h>
#include <Statme/Macros.h>

namespace Duration
{
  class Meter;
  struct STATMELNK Calculator
  {
    Meter* Owner;
    MeasurementPtr Parent;
    uint64_t Start;
    uint64_t ThreadID;

  public:
    Calculator(Meter* meter, MeasurementPtr parent);
    ~Calculator();
  };

  typedef std::shared_ptr<Calculator> CalculatorPtr;
}