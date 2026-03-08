#include "cannect/CanDispatcher.hpp"

#include "cannect/Logger.hpp"

namespace cannect
{

CanDispatcher::~CanDispatcher() { stop(); }

Status CanDispatcher::addInterface(std::shared_ptr<ICanInterface> canInterface)
{
    LOG_DEBUG("Adding interface " + canInterface->getName());
    if (!canInterface)
    {
        LOG_ERROR("CAN interface is nullptr");
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    if (running)
    {
        LOG_ERROR("Dispatcher is not running");
        return Status::UNSUCCESS;
    }

    if (findEntry(canInterface->getName()))
    {
        LOG_ERROR("CAN interface already exists.");
        return Status::INTERFACE_ALREADY_OPEN;
    }

    LOG_INFO("Successfuly added CAN interface " + canInterface->getName());
    dispatcherEntries.push_back({std::move(canInterface), {}, {}, nullptr});
    return Status::SUCCESS;
}

Status CanDispatcher::addReceiver(const std::string &interfaceName, std::shared_ptr<ICanFrameHandler> receiver)
{
    LOG_DEBUG("Adding CAN frame receiver for " + interfaceName);

    if (!receiver)
    {
        LOG_ERROR("CAN receiver is nullptr");
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    DispatcherEntry *entry = findEntry(interfaceName);
    if (!entry)
    {
        LOG_ERROR("Could not find CAN interface " + interfaceName);
        return Status::INTERFACE_INVALID_NAME;
    }

    LOG_INFO("Added CAN receiver to " + interfaceName);
    entry->receivers.push_back(std::move(receiver));
    return Status::SUCCESS;
}

Status CanDispatcher::addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter)
{
    LOG_DEBUG("Adding CAN frame receiver for " + interfaceName);

    if (!filter)
    {
        LOG_ERROR("Filter for " + interfaceName + "is nullptr");
        return Status::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(entriesMutex);

    DispatcherEntry *entry = findEntry(interfaceName);
    if (!entry)
    {
        LOG_ERROR("Could not find CAN interface " + interfaceName);
        return Status::INTERFACE_INVALID_NAME;
    }
    LOG_INFO("Added filter to " + interfaceName);
    entry->filters.push_back(std::move(filter));
    return Status::SUCCESS;
}

void CanDispatcher::start()
{
    std::lock_guard<std::mutex> lock(entriesMutex);

    if (running)
    {
        LOG_WARNING("Dispatcher is already running");
        return;
    }

    running = true;

    for (DispatcherEntry &entry : dispatcherEntries)
    {
        if (entry.canInterface->open() != Status::SUCCESS)
        {
            LOG_WARNING("Failed to open " + entry.canInterface->getName());
            continue;
        }
        entry.ifaceThread = std::make_unique<std::thread>([this, &entry]() { runInterface(entry); });
        LOG_DEBUG("Created thread for " + entry.canInterface->getName());
    }
}

void CanDispatcher::stop()
{
    {
        std::lock_guard<std::mutex> lock(entriesMutex);

        if (!running)
        {
            LOG_WARNING("Dispatcher is already not running");
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
    LOG_INFO("Stopped CAN dispatcher");
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
            LOG_WARNING("Failed to receive CAN frame");
            continue;
        }

        bool accepted = true;
        for (const std::shared_ptr<IFilter> &filter : entry.filters)
        {
            if (!filter->isValid(frame))
            {
                LOG_DEBUG("Frame for " + entry.canInterface->getName() + "discarded");
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
