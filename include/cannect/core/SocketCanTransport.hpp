#pragma once
#include <cstdint>
#include <string>
#include <sys/time.h>
#include <vector>

#include "CanFrame.hpp"
#include "ICanTransport.hpp"

namespace cannect
{

    struct CanFilter
    {
        uint32_t id;
        uint32_t mask;

        CanFilter(uint32_t filterId = 0, uint32_t filterMask = 0)
            : id(filterId)
            , mask(filterMask)
        {
        }
    };

    class SocketCanTransport : public ICanTransport
    {
      public:
        SocketCanTransport()
            : m_interface("")
            , m_isOpen(false)
        {
        }
        explicit SocketCanTransport(const std::string &interface)
            : m_interface(interface)
            , m_isOpen(false)
        {
        }

        ~SocketCanTransport() override;

        bool open() override;
        bool close() override;
        bool isOpen() const override
        {
            return m_isOpen;
        }

        int writeFrame(const CanFrame &frame) override;
        int readFrame(CanFrame &frame) override;

        bool               setReadTimeout(int milliseconds);
        bool               setFilters(const std::vector<CanFilter> &filters);
        void               setInterface(const std::string &interface);
        const std::string &getInterface() const
        {
            return m_interface;
        }

      private:
        std::string m_interface;
        bool        m_isOpen;
        int         fd = -1;
    };

} // namespace cannect
