/**
 * @file cants/CanTsProtocol.hpp
 * @brief Declares the CAN-TS protocol adapter built on top of raw CAN frames.
 */

#pragma once

#include "GetBlockManager.hpp"
#include "SetBlockManager.hpp"
#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

/**
 * @brief Default timeout, in milliseconds, used by synchronous CAN-TS transactions.
 */
#define DEFAULT_TIMEOUT_MS 200

/**
 * @brief Reserved destination address used by keep-alive frames.
 */
#define CAN_TS_KEEPALIVE_ADDRESS 1

namespace cannect
{

/**
 * @brief Implements the CAN-TS application protocol on top of CAN frames.
 */
class CanTsProtocol : public ICanProtocol
{
  public:
    /**
     * @brief Creates a CAN-TS endpoint for one local node address.
     *
     * @param localAddress Local CAN-TS node address.
     */
    explicit CanTsProtocol(uint8_t localAddress);

    /**
     * @brief Handles inbound telecommand requests.
     *
     * Return `true` to acknowledge the request, or `false` to send a negative acknowledgement.
     */
    using TcHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t request[CAN_FRAME_MAX_DATA])>;

    /**
     * @brief Produces telemetry returned in response to a telemetry request.
     *
     * The handler should fill the provided buffer and return `true` to acknowledge the request.
     */
    using TmHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t response[CAN_FRAME_MAX_DATA])>;

    /**
     * @brief Receives unsolicited telemetry frames.
     */
    using UTMHandler = std::function<void(uint8_t from, uint8_t channel, uint8_t data[CAN_FRAME_MAX_DATA])>;

    /**
     * @brief Receives time synchronization payloads.
     */
    using TimeSyncHandler = std::function<void(uint8_t from, uint8_t timeLE[CAN_FRAME_MAX_DATA])>;

    /**
     * @brief Alias for the SETBLOCK request handler managed by SetBlockManager.
     */
    using SetBlockHandler = SetBlockManager::Handler;

    /**
     * @brief Alias for the GETBLOCK request handler managed by GetBlockManager.
     */
    using GetBlockHandler = GetBlockManager::Handler;

    /**
     * @brief Decodes and routes one inbound CAN frame.
     *
     * @param frame Incoming CAN frame.
     * @return Status::SUCCESS when the frame is accepted for processing, otherwise an error code.
     */
    Status onFrame(const CanFrame &frame) override;

    /**
     * @brief Sets the transport used for outbound protocol frames.
     *
     * @param transmitter Transport used to send encoded CAN-TS frames.
     * @return Status::SUCCESS when the transmitter is valid, otherwise Status::INVALID_ARGUMENT.
     */
    Status setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter) override;

    /**
     * @brief Registers the telecommand request handler.
     *
     * @param handler Callback invoked for inbound telecommands.
     */
    void setTelecommandHandler(TcHandler handler);

    /**
     * @brief Registers the telemetry request handler.
     *
     * @param handler Callback invoked for inbound telemetry requests.
     */
    void setTelemetryHandler(TmHandler handler);

    /**
     * @brief Registers the unsolicited telemetry handler.
     *
     * @param handler Callback invoked for inbound unsolicited telemetry.
     */
    void setUnsolicitedTelemetryHandler(UTMHandler handler);

    /**
     * @brief Registers the time synchronization handler.
     *
     * @param handler Callback invoked for inbound time synchronization frames.
     */
    void setTimeSyncHandler(TimeSyncHandler handler);

    /**
     * @brief Registers the handler for inbound SETBLOCK transactions.
     *
     * @param handler Callback invoked after all SETBLOCK data is reassembled.
     */
    void setSetBlockHandler(SetBlockHandler handler);

    /**
     * @brief Registers the handler for inbound GETBLOCK transactions.
     *
     * @param handler Callback used to provide outbound GETBLOCK data.
     */
    void setGetBlockHandler(GetBlockHandler handler);

    /**
     * @brief Sends a telecommand and waits for an acknowledgement.
     *
     * @param to Destination node address.
     * @param channel Application-defined telecommand channel.
     * @param data Fixed-size request payload.
     * @return true when the peer acknowledges the telecommand, otherwise false.
     */
    bool sendTelecommand(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);

    /**
     * @brief Requests telemetry from a peer and waits for the response.
     *
     * The last successful response can be retrieved later with getLastTelemetry().
     *
     * @param to Destination node address.
     * @param channel Application-defined telemetry channel.
     * @return true when a telemetry acknowledgement is received, otherwise false.
     */
    bool requestTelemetry(uint8_t to, uint8_t channel);

    /**
     * @brief Sends unsolicited telemetry without waiting for a response.
     *
     * @param to Destination node address.
     * @param channel Application-defined unsolicited telemetry channel.
     * @param data Fixed-size telemetry payload.
     * @return true when the frame is transmitted, otherwise false.
     */
    bool sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);

    /**
     * @brief Broadcasts a time synchronization payload.
     *
     * @param timeLE Time payload encoded in little-endian byte order.
     * @return true when the frame is transmitted, otherwise false.
     */
    bool broadcastTimeSync(const uint8_t timeLE[CAN_FRAME_MAX_DATA]);

    /**
     * @brief Sends a block-write transaction through the internal SetBlockManager.
     *
     * @param to Destination node address.
     * @param addressLE Little-endian encoded block address or selector.
     * @param data Payload to write.
     * @param timeoutMs Timeout applied to the synchronous transaction.
     * @return true when the transaction succeeds, otherwise false.
     */
    bool setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                  uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);

    /**
     * @brief Sends a block-read transaction through the internal GetBlockManager.
     *
     * @param to Destination node address.
     * @param addressLE Little-endian encoded block address or selector.
     * @param timeoutMs Timeout applied to the synchronous transaction.
     * @return true when the transaction succeeds, otherwise false.
     */
    bool getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);

    /**
     * @brief Returns the most recent telemetry payload received by requestTelemetry().
     *
     * @param out Output buffer that receives the cached telemetry data.
     * @param dlc Output length of the cached payload.
     * @return true when cached telemetry is available, otherwise false.
     */
    bool getLastTelemetry(uint8_t out[CAN_FRAME_MAX_DATA], uint8_t &dlc) const;

    /**
     * @brief Returns the payload from the most recent successful GETBLOCK transaction.
     *
     * @param out Output buffer that receives the cached block data.
     * @return true when cached data is available, otherwise false.
     */
    bool getLastGetBlockData(std::vector<uint8_t> &out) const;

    /**
     * @brief Checks whether a peer has been seen recently on the bus.
     *
     * Any successfully decoded frame from the peer refreshes its activity timestamp.
     *
     * @param peer Peer node address.
     * @param timeoutMs Maximum age of the last-seen timestamp.
     * @return true when the peer is considered connected, otherwise false.
     */
    bool isPeerConnected(uint8_t peer, uint32_t timeoutMs) const;

    /**
     * @brief Sends an unsolicited telemetry frame to the reserved keep-alive address.
     *
     * @param channel Application-defined keep-alive channel.
     * @param data Fixed-size keep-alive payload.
     * @return true when the frame is transmitted, otherwise false.
     */
    bool sendKeepAlive(uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);

  private:
    struct PendingTcTm
    {
        bool waiting = false;
        bool done = false;
        bool ok = false;
        uint8_t peer = 0;
        uint8_t channel = 0;
        uint8_t data[CAN_FRAME_MAX_DATA] = {};
        uint8_t dlc = 0;
    };

    struct PeerState
    {
        std::chrono::steady_clock::time_point lastSeen{};
    };

    void handleTelecommand(const CanTsHeader &h, const CanFrame &frame);
    void handleTelemetry(const CanTsHeader &h, const CanFrame &frame);
    void handleUnsolicited(const CanTsHeader &h, const CanFrame &frame);
    void handleTimeSync(const CanTsHeader &h, const CanFrame &frame);

    bool sendFrame(const CanTsHeader &h, const uint8_t *data, uint8_t dlc);
    void notePeerAlive(uint8_t peer);

    mutable std::mutex mtx;
    std::condition_variable cv;

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;

    std::unordered_map<uint8_t, PeerState> peers;

    PendingTcTm pendingTc;
    PendingTcTm pendingTm;

    std::shared_ptr<ICanFrameTransmitter> transmitter;
    uint8_t localAddress = 0;

    SetBlockManager setBlockMgr;
    GetBlockManager getBlockMgr;

    uint8_t lastTelemetry[CAN_FRAME_MAX_DATA] = {};
    uint8_t lastTelemetryDlc = 0;
};

} // namespace cannect
