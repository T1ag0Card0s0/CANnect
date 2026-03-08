#include "cannect/CanDispatcher.hpp"

namespace cannect
{

CanDispatcher::~CanDispatcher() { stop(); }

Status CanDispatcher::addInterface(std::shared_ptr<ICanInterface> canInterface)
{
    if (!canInterface)
    {
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    if (running)
    {
        return Status::UNSUCCESS;
    }

    if (findEntry(canInterface->getName()))
    {
        return Status::INTERFACE_ALREADY_OPEN;
    }

    dispatcherEntries.push_back({std::move(canInterface), {}, {}, nullptr});
    return Status::SUCCESS;
}

Status CanDispatcher::addReceiver(const std::string &interfaceName, std::shared_ptr<ICanFrameHandler> receiver)
{
    if (!receiver)
    {
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    DispatcherEntry *entry = findEntry(interfaceName);
    if (!entry)
    {
        return Status::INTERFACE_INVALID_NAME;
    }

    entry->receivers.push_back(std::move(receiver));
    return Status::SUCCESS;
}

Status CanDispatcher::addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter)
{
    if (!filter)
    {
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    DispatcherEntry *entry = findEntry(interfaceName);
    if (!entry)
    {
        return Status::INTERFACE_INVALID_NAME;
    }

    entry->filters.push_back(std::move(filter));
    return Status::SUCCESS;
}

void CanDispatcher::start()
{
    std::lock_guard<std::mutex> lock(entriesMutex);

    if (running)
    {
        return;
    }

    running = true;

    for (DispatcherEntry &entry : dispatcherEntries)
    {
        if (entry.canInterface->open() != Status::SUCCESS)
        {
            continue;
        }

        entry.ifaceThread = std::make_unique<std::thread>([this, &entry]() { runInterface(entry); });
    }
}

void CanDispatcher::stop()
{
    {
        std::lock_guard<std::mutex> lock(entriesMutex);

        if (!running)
        {
            return;
        }

        running = false;

        for (DispatcherEntry &entry : dispatcherEntries)
        {
            entry.canInterface->close();
        }
    }

    for (DispatcherEntry &entry : dispatcherEntries)
    {
        if (entry.ifaceThread && entry.ifaceThread->joinable())
        {
            entry.ifaceThread->join();
        }
    }
}

CanDispatcher::DispatcherEntry *CanDispatcher::findEntry(const std::string &interfaceName)
{
    for (DispatcherEntry &entry : dispatcherEntries)
    {
        if (entry.canInterface->getName() == interfaceName)
        {
            return &entry;
        }
    }
    return nullptr;
}

void CanDispatcher::runInterface(DispatcherEntry &entry)
{
    while (running)
    {
        CanFrame frame;
        if (entry.canInterface->receive(frame) != Status::SUCCESS)
        {
            break;
        }

        bool accepted = true;
        for (const std::shared_ptr<IFilter> &filter : entry.filters)
        {
            if (!filter->isValid(frame))
            {
                accepted = false;
                break;
            }
        }

        if (!accepted)
        {
            continue;
        }

        for (const std::shared_ptr<ICanFrameHandler> &receiver : entry.receivers)
        {
            receiver->onFrame(frame);
        }
    }
}

} // namespace cannect
