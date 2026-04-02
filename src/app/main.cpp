#include "Cli.hpp"
#include "Handlers.hpp"
#include "cannect/Cannect.hpp"
#include "cannect/LogSinks.hpp"
#include "cannect/Logger.hpp"
#include "cannect/cants/CanTsProtocol.hpp"

#include <iostream>
#include <memory>

using namespace cannect;

int main(int argc, char *argv[])
{
    try
    {
        Logger::instance()->addSink(std::make_shared<ConsoleSink>());

        const Options options = parseOptions(argc, argv);

        if (options.showHelp)
        {
            printUsage(argv[0]);
            return 0;
        }

        if (options.ifaceSpec.empty())
        {
            std::cerr << "No CAN interface specified. Use -i / --iface.\n";
            printUsage(argv[0]);
            return 1;
        }

        if (!options.outputFile.empty())
        {
            Logger::instance()->addSink(std::make_shared<FileSink>(options.outputFile));
        }

        auto iface = createInterface(options.ifaceSpec);

        Cannect app;
        auto proto = std::make_shared<CanTsProtocol>(options.localNode);

        proto->setTelecommandHandler(onTelecommand);
        proto->setTelemetryHandler(onTelemetry);
        proto->setUnsolicitedTelemetryHandler(onUnsolicitedTelemetry);
        proto->setTimeSyncHandler(onTimeSync);
        proto->setSetBlockHandler(onSetBlock);
        proto->setGetBlockHandler(onGetBlock);
        proto->setFrameTransmitter(iface);

        if (app.addInterface(iface) != Status::SUCCESS)
        {
            std::cerr << "Failed to add interface\n";
            return 1;
        }

        if (app.addHandler(iface->getName(), proto) != Status::SUCCESS)
        {
            std::cerr << "Failed to add protocol handler\n";
            return 1;
        }

        app.start();

        if (options.hasSendCommand)
        {
            executeSendCommand(*proto, argc, argv, options.sendArgIndex);
        }
        else
        {
            std::cout << "CANnect listening on " << iface->getName() << " as node "
                      << static_cast<int>(options.localNode) << "\n";
        }

        app.waitSignal();
        app.stop();
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << "\n";
        printUsage(argv[0]);
        return 1;
    }
}
