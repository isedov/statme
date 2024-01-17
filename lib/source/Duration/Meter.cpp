#include <cassert>

#include <Statme/Duration/Meter.h>
#include <Syncme/ProcessThreadId.h>

using namespace Syncme;
using namespace Duration;

Meter::Meter()
{
}

Meter::Handle* Meter::StartMeasurement2(DurationCounter& dc, const char* name)
{
  Handle* h = new Handle;
  h->Calculator = StartMeasurement(dc, name);
  return h;
}

CalculatorPtr Meter::StartMeasurement(DurationCounter& dc, const char* name)
{
  auto id = GetCurrentThreadId();
  auto lock = Lock.Lock();

  MeasurementPtr parent;

  auto it = Map.find(id);
  if (it == Map.end())
  {
    ThreadDataPtr data = std::make_shared<ThreadData>();
    data->Root = std::make_shared<Measurement>(dc, name);
    data->Pos = data->Root;

    Map[id] = data;
  }
  else
  {
    ThreadData& data = *it->second;
    
    if (data.Pos == nullptr)
    {
      // Replace completed root measurement

      data.Root.reset();
      data.Root = std::make_shared<Measurement>(dc, name);
      data.Pos = data.Root;
    }
    else
    {
      parent = data.Pos;

      MeasurementPtr measurement = std::make_shared<Measurement>(dc, name);
      parent->Child.push_back(measurement);

      data.Pos.reset();
      data.Pos = measurement;
    }
  }

  return std::make_shared<Calculator>(this, parent);
}

void Meter::StopMeasurement(
  uint64_t id
  , uint64_t duration
  , MeasurementPtr parent
)
{
  auto lock = Lock.Lock();

  auto it = Map.find(id);
  if (it == Map.end())
    return;
  
  if (it->second->Pos)
  {
    it->second->Pos->Duration = duration;
    it->second->Pos->Counter.Add(duration);

    it->second->Pos.reset();
    it->second->Pos = parent;
  }
}

ThreadDataPtr Meter::PopThreadData()
{
  auto id = GetCurrentThreadId();
  Lock.Acquire();

  auto it = Map.find(id);
  if (it == Map.end())
  {
    Lock.Release();
    return ThreadDataPtr();
  }

  ThreadDataPtr data = it->second;
  Map.erase(it);

  Lock.Release();
  return data;
}

void Meter::AppendData(
  std::string& str
  , const std::string& align
  , MeasurementPtr m
)
{
  str += "\n";
  str += align;
  str += m->Name;
  str += "(): ";
  str += std::to_string(m->Duration);

  for (auto& c : m->Child)
    AppendData(str, align + "  ", c);
}

void Meter::PrintResults(
  const Logme::ID& ch
  , const std::string& title
)
{
  ThreadDataPtr data = PopThreadData();
  if (data == nullptr)
    return;

  std::string str;
  str += "  [";
  str += title;
  str += "]";
  AppendData(str, "    ", data->Root);

  Logme::Override ovr;
  ovr.Remove.Method = true;
  ovr.Remove.ErrorPrefix = true;

  LogmeI(ch, ovr, "\n%s", str.c_str());
}
