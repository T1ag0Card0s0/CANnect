#include <memory>

int main() {
    using namespace can;

    VirtualCanInterface ifaceA{"can0"};
    VirtualCanInterface ifaceB{"can1"};

    InterfaceConfig cfg;
    cfg.nominalBitrate = 500000;

    ifaceA.initialize(cfg);
    ifaceB.initialize(cfg);
    ifaceA.start();
    ifaceB.start();

    CanRouter router;
    router.registerInterface(ifaceA);
    router.registerInterface(ifaceB);

    auto protocol = std::make_unique<SimpleCanOpenProtocol>(0x01);
    router.registerProtocol(std::move(protocol));

    router.addRoute(
        {"can0", std::make_shared<AcceptAllFilter>()},
        {"can1", std::make_shared<AcceptAllFilter>()}
    );

    CanFrame heartbeat;
    heartbeat.id = {0x701, FrameFormat::Standard};
    heartbeat.size = 1;
    heartbeat.payload[0] = 0x05;
    heartbeat.timestamp = Clock::now();

    ifaceA.inject(heartbeat);

    router.tick();
}
