#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanInterface.hpp"
#include <thread>
#include <vector>

namespace cannect
{
  class IFilter
  {
    public:
      virtual ~IFilter() = default;
      virtual bool isValid() = 0;
  };

  class CanDispatcher
  {
    public: 
      CanDispatcher() = default;
      ~CanDispatcher() = default;

      Status addInterface(ICanInterface &canInterface);
      Status addReceiver(std::string name, ICanFrameHandler &canReceiver);
      Status addFilter(std::string name, IFilter filter);
      
      void start();
      void stop();

    private:
      struct DispatcherEntry
      {
        std::vector<IFilter> filters;
        std::vector<ICanFrameHandler> receivers;
        ICanInterface &canInterface;
        std::thread ifaceThread;
      };

      std::vector<DispatcherEntry> dispatcherEntries;
  };
}
