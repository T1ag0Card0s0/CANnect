#include "cannect/cants/GetBlockManager.hpp"

#include "CanTsCodec.hpp"
#include "cannect/Logger.hpp"

#include <algorithm>
#include <cstring>

using namespace cannect;

GetBlockManager::GetBlockManager(uint8_t localAddress, FrameSender sender)
    : localAddress(localAddress), frameSender(std::move(sender))
{
}

void GetBlockManager::setHandler(Handler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    handler = std::move(h);
}

void GetBlockManager::onFrame(const CanTsHeader &h, const CanFrame &frame)
{
    const GetBlockFrameType type = CanTsCodec::getGetBlockType(h.command);

    switch (type)
    {
    case GetBlockFrameType::REQUEST: {
        const uint8_t numBlocks = static_cast<uint8_t>(CanTsCodec::getGetBlockLow6(h.command) + 1u);

        Handler handlerCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            handlerCopy = handler;
        }

        std::vector<uint8_t> responseData;
        bool ok = false;

        if (handlerCopy)
        {
            ok = handlerCopy(h.from, 0, responseData);
        }

        if (!ok || numBlocks == 0 || numBlocks > 64 || responseData.size() > CAN_TS_MAX_BLOCK_BYTES ||
            responseData.size() != static_cast<size_t>(numBlocks) * CAN_FRAME_MAX_DATA || frame.dlc == 0)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::GETBLOCK;
            nack.command =
                CanTsCodec::makeGetBlockCommand(GetBlockFrameType::NACK, CanTsCodec::ackCommandLow7(h.command));
            (void)frameSender(nack, frame.data, frame.dlc);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);

            Session session{};
            session.peer = h.from;
            session.channel = 0;
            session.address.assign(frame.data, frame.data + frame.dlc);
            session.numBlocks = numBlocks;
            session.blocks.resize(numBlocks);
            session.lastActivity = std::chrono::steady_clock::now();
            session.prepared = true;
            session.accepted = true;

            for (uint8_t i = 0; i < numBlocks; ++i)
            {
                session.blocks[i].assign(responseData.begin() + (i * CAN_FRAME_MAX_DATA),
                                         responseData.begin() + ((i + 1) * CAN_FRAME_MAX_DATA));
            }

            sessions[h.from] = std::move(session);
        }

        CanTsHeader ack{};
        ack.to = h.from;
        ack.from = localAddress;
        ack.type = CanTsMessageType::GETBLOCK;
        ack.command = CanTsCodec::makeGetBlockCommand(GetBlockFrameType::ACK, CanTsCodec::ackCommandLow7(h.command));

        (void)frameSender(ack, frame.data, frame.dlc);
        break;
    }

    case GetBlockFrameType::START: {
        std::vector<std::vector<uint8_t>> blocksToSend;
        uint8_t numBlocks = 0;
        bool valid = false;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sessions.find(h.from);
            if (it != sessions.end() && it->second.accepted)
            {
                const std::vector<uint8_t> bitmap(frame.data, frame.data + frame.dlc);
                it->second.lastActivity = std::chrono::steady_clock::now();

                numBlocks = it->second.numBlocks;
                blocksToSend = it->second.blocks;
                valid = true;

                for (uint8_t i = 0; i < numBlocks; ++i)
                {
                    if (!CanTsCodec::bitmapBitIsSet(bitmap, i))
                    {
                        blocksToSend[i].clear();
                    }
                }
            }
        }

        if (!valid)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::GETBLOCK;
            nack.command =
                CanTsCodec::makeGetBlockCommand(GetBlockFrameType::NACK, CanTsCodec::ackCommandLow7(h.command));
            (void)frameSender(nack, frame.data, frame.dlc);
            return;
        }

        for (uint8_t i = 0; i < numBlocks; ++i)
        {
            if (blocksToSend[i].empty())
            {
                continue;
            }

            CanTsHeader transfer{};
            transfer.to = h.from;
            transfer.from = localAddress;
            transfer.type = CanTsMessageType::GETBLOCK;
            transfer.command = CanTsCodec::makeGetBlockCommand(GetBlockFrameType::TRANSFER, i);

            const uint8_t dlc = static_cast<uint8_t>(std::min<size_t>(blocksToSend[i].size(), CAN_FRAME_MAX_DATA));
            (void)frameSender(transfer, blocksToSend[i].data(), dlc);
        }
        break;
    }

    case GetBlockFrameType::ABORT: {
        std::lock_guard<std::mutex> lock(mtx);
        sessions.erase(h.from);
        break;
    }

    case GetBlockFrameType::ACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pending.waitingAck && pending.peer == h.from)
        {
            pending.acked = true;
            pending.waitingAck = false;
            cv.notify_all();
        }
        break;
    }

    case GetBlockFrameType::NACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if ((pending.waitingAck || pending.waitingTransfers) && pending.peer == h.from)
        {
            pending.done = true;
            pending.ok = false;
            cv.notify_all();
        }
        break;
    }

    case GetBlockFrameType::TRANSFER: {
        std::lock_guard<std::mutex> lock(mtx);
        if (!pending.waitingTransfers || pending.peer != h.from)
        {
            return;
        }

        const uint8_t seq = CanTsCodec::getGetBlockLow6(h.command);
        if (seq >= pending.numBlocks)
        {
            return;
        }

        pending.blocks[seq].assign(frame.data, frame.data + frame.dlc);
        pending.received[seq] = true;

        if (CanTsCodec::allBlocksReceived(pending.received))
        {
            pending.done = true;
            pending.ok = true;

            lastData.clear();
            for (uint8_t i = 0; i < pending.numBlocks; ++i)
            {
                lastData.insert(lastData.end(), pending.blocks[i].begin(), pending.blocks[i].end());
            }

            cv.notify_all();
        }
        break;
    }

    default:
        break;
    }
}

bool GetBlockManager::getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs)
{
    if (addressLE.empty())
    {
        return false;
    }

    // This still assumes a fixed 64-block transfer shape.
    // If you want a general solution, the protocol needs a size negotiation step.
    const uint8_t numBlocks = 64;

    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        pending.waitingAck = true;
        pending.peer = to;
        pending.numBlocks = numBlocks;
    }

    CanTsHeader req{};
    req.to = to;
    req.from = localAddress;
    req.type = CanTsMessageType::GETBLOCK;
    req.command = CanTsCodec::makeGetBlockCommand(GetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1u));

    if (!frameSender(req, addressLE.data(),
                     static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool acked =
            cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return pending.acked || pending.done; });

        if (!acked || pending.done)
        {
            pending = {};
            return false;
        }

        pending.waitingTransfers = true;
        pending.blocks.assign(numBlocks, {});
        pending.received.assign(numBlocks, false);
    }

    std::vector<uint8_t> bitmap(CanTsCodec::bitmapSizeBytes(numBlocks), 0xFFu);
    if (!bitmap.empty())
    {
        const uint8_t extraBits = static_cast<uint8_t>((bitmap.size() * 8u) - numBlocks);
        if (extraBits > 0)
        {
            bitmap.back() &= static_cast<uint8_t>(0xFFu >> extraBits);
        }
    }

    CanTsHeader start{};
    start.to = to;
    start.from = localAddress;
    start.type = CanTsMessageType::GETBLOCK;
    start.command = CanTsCodec::makeGetBlockCommand(GetBlockFrameType::START, 0);

    if (!frameSender(start, bitmap.data(), static_cast<uint8_t>(std::min<size_t>(bitmap.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        return false;
    }

    bool success = false;
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool done = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return pending.done; });
        success = done && pending.ok;
        pending = {};
    }

    return success;
}

bool GetBlockManager::getLastData(std::vector<uint8_t> &out) const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (lastData.empty())
    {
        return false;
    }

    out = lastData;
    return true;
}
