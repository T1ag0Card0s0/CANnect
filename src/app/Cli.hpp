#pragma once

#include "cannect/ICanInterface.hpp"
#include "cannect/cants/CanTsProtocol.hpp"

#include <cstdint>
#include <memory>
#include <string>

struct Options
{
    std::string ifaceSpec;
    std::string outputFile;
    uint8_t localNode = 65;
    bool showHelp = false;
    bool hasSendCommand = false;
    int sendArgIndex = -1;
};

uint8_t parseU8(const char *s);
uint32_t parseU32(const char *s);

void printUsage(const char *prog);
Options parseOptions(int argc, char *argv[]);

std::shared_ptr<cannect::ICanInterface> createInterface(const std::string &spec);
void executeSendCommand(cannect::CanTsProtocol &proto, int argc, char *argv[], int startIndex);
