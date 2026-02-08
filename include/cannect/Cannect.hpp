#pragma once

#include "cannect/cli/ArgumentParser.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cannect/core/IProtocol.hpp"

#include <memory.h>
#include <memory>

namespace cannect
{

    class Cannect
    {
      public:
        Cannect();
        ~Cannect();

        int run(int argc, char **argv);

      private:
        ArgumentParser argumentParser;
        std::shared_ptr<ICanTransport> socket;
        std::shared_ptr<ICanObserver> observer;
        std::shared_ptr<IProtocol> protocol;
    };

} // namespace cannect
