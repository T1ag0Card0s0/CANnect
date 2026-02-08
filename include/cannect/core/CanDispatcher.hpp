#pragma once

#include "ICanObserver.hpp"

#include <list>
#include <memory>

namespace cannect
{

    class CanDispatcher
    {
      public:
        CanDispatcher() = default;
        ~CanDispatcher() = default;
        void attach(std::shared_ptr<ICanObserver> canObserver);
        void detach(std::shared_ptr<ICanObserver> canObserver);
        void notify(CanFrame &canFrame);

      private:
        std::list<std::shared_ptr<ICanObserver>> observers;
    };

} // namespace cannect
