#pragma once

#include <memory>
#include <vector>

#include "cannect/cli/ArgumentParser.hpp"
#include "cannect/core/CanListener.hpp"
#include "cannect/core/ICanObserver.hpp"
#include "cannect/core/ICanTransport.hpp"

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

        std::unique_ptr<ICanTransport>             socket;
        std::unique_ptr<CanListener>               listener;
        std::vector<std::unique_ptr<ICanObserver>> observers;
    };

} // namespace cannect
