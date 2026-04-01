#include "cannect/cants/SetBlockManager.hpp"

#include "CanTsCodec.hpp"
#include "cannect/Logger.hpp"

#include <algorithm>
#include <cstring>

using namespace cannect;
using C = CanTsCodec;

SetBlockManager::SetBlockManager(uint8_t localAddress, FrameSender sender)
    : localAddress(localAddress), frameSender(std::move(sender))
{
}

void SetBlockManager::setHandler(Handler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    handler = std::move(h);
}

void SetBlockManager::onFrame(const CanTsHeader &h, const CanFrame &frame)
{
    const SetBlockFrameType type = C::getSetBlockType(h.command);

    switch (type)
    {
    case SetBlockFrameType::REQUEST: {
        const uint8_t numBlocks = static_cast<uint8_t>(C::getSetBlockLow6(h.command) + 1u);

        bool accepted = false;
        {
            std::lock_guard<std::mutex> lock(mtx);

            Session session{};
            session.peer = h.from;
            session.channel = 0;
            session.address.assign(frame.data, frame.data + frame.dlc);
            session.numBlocks = numBlocks;
            session.blocks.assign(numBlocks, {});
            session.lastActivity = std::chrono::steady_clock::now();
            session.prepared = true;
            session.done = false;
            session.accepted = (numBlocks > 0 && numBlocks <= 64 &&
                                static_cast<size_t>(numBlocks) * CAN_FRAME_MAX_DATA <= CAN_TS_MAX_BLOCK_BYTES &&
                                !session.address.empty());

            accepted = session.accepted;
            sessions[h.from] = std::move(session);
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::SETBLOCK;
        resp.command = C::makeSetBlockCommand(accepted ? SetBlockFrameType::ACK : SetBlockFrameType::NACK,
                                              C::ackCommandLow7(h.command));

        (void)frameSender(resp, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::TRANSFER: {
        bool accept = false;
        uint8_t seq = C::getSetBlockLow6(h.command);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sessions.find(h.from);
            if (it != sessions.end() && it->second.accepted && seq < it->second.numBlocks)
            {
                it->second.blocks[seq].assign(frame.data, frame.data + frame.dlc);
                it->second.lastActivity = std::chrono::steady_clock::now();
                accept = true;
            }
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::SETBLOCK;
        resp.command = C::makeSetBlockCommand(accept ? SetBlockFrameType::ACK : SetBlockFrameType::NACK,
                                              C::ackCommandLow7(h.command));

        (void)frameSender(resp, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::STATUS_REQUEST: {
        Handler handlerCopy;
        std::vector<uint8_t> assembled;
        std::vector<uint8_t> bitmap;
        bool haveSession = false;
        bool accepted = false;
        bool alreadyDone = false;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sessions.find(h.from);
            if (it != sessions.end())
            {
                haveSession = true;
                accepted = it->second.accepted;
                alreadyDone = it->second.done;
                handlerCopy = handler;

                if (accepted)
                {
                    std::vector<bool> received(it->second.numBlocks, false);
                    for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                    {
                        received[i] = !it->second.blocks[i].empty();
                    }

                    bitmap = C::makeBitmap(received);

                    if (C::allBlocksReceived(received) && !it->second.done)
                    {
                        assembled.reserve(static_cast<size_t>(it->second.numBlocks) * CAN_FRAME_MAX_DATA);
                        for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                        {
                            assembled.insert(assembled.end(), it->second.blocks[i].begin(), it->second.blocks[i].end());
                        }
                    }
                }
            }
        }

        if (!haveSession || !accepted)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::SETBLOCK;
            nack.command = C::makeSetBlockCommand(SetBlockFrameType::NACK, C::ackCommandLow7(h.command));
            (void)frameSender(nack, frame.data, frame.dlc);
            return;
        }

        bool handlerDone = alreadyDone;
        if (!assembled.empty() && !alreadyDone)
        {
            if (handlerCopy)
            {
                std::vector<uint8_t> request = assembled;
                handlerDone = handlerCopy(h.from, 0, request);
            }
            else
            {
                handlerDone = true;
            }

            std::lock_guard<std::mutex> lock(mtx);
            auto it = sessions.find(h.from);
            if (it != sessions.end())
            {
                it->second.done = handlerDone;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sessions.find(h.from);
            if (it != sessions.end())
            {
                std::vector<bool> received(it->second.numBlocks, false);
                for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                {
                    received[i] = !it->second.blocks[i].empty();
                }
                bitmap = C::makeBitmap(received);
                handlerDone = it->second.done;
            }
        }

        CanTsHeader report{};
        report.to = h.from;
        report.from = localAddress;
        report.type = CanTsMessageType::SETBLOCK;
        report.command =
            C::makeSetBlockReportCommand(static_cast<uint8_t>(bitmap.size() & C::BLOCK_LOW6_MASK), handlerDone);

        const uint8_t dlc = static_cast<uint8_t>(std::min<size_t>(bitmap.size(), CAN_FRAME_MAX_DATA));
        (void)frameSender(report, bitmap.data(), dlc);
        break;
    }

    case SetBlockFrameType::ABORT: {
        {
            std::lock_guard<std::mutex> lock(mtx);
            sessions.erase(h.from);
        }

        CanTsHeader ack{};
        ack.to = h.from;
        ack.from = localAddress;
        ack.type = CanTsMessageType::SETBLOCK;
        ack.command = C::makeSetBlockCommand(SetBlockFrameType::ACK, C::ackCommandLow7(h.command));

        (void)frameSender(ack, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::ACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pending.active && pending.peer == h.from && pending.waitingRequestAck)
        {
            pending.requestAcked = true;
            pending.waitingRequestAck = false;
            cv.notify_all();
        }
        break;
    }

    case SetBlockFrameType::NACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pending.active && pending.peer == h.from)
        {
            if (pending.waitingRequestAck || pending.waitingReport)
            {
                pending.done = true;
                pending.ok = false;
                cv.notify_all();
            }
        }
        break;
    }

    case SetBlockFrameType::REPORT: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pending.active && pending.peer == h.from && pending.waitingReport)
        {
            pending.bitmap.assign(frame.data, frame.data + frame.dlc);
            pending.done = C::getSetBlockDone(h.command);
            pending.ok = true;
            pending.reportReceived = true;
            cv.notify_all();
        }
        break;
    }

    default:
        break;
    }
}

bool SetBlockManager::setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                               uint32_t timeoutMs)
{
    if (addressLE.empty() || data.empty() || data.size() > CAN_TS_MAX_BLOCK_BYTES)
    {
        return false;
    }

    const size_t numBlocksSz = (data.size() + CAN_FRAME_MAX_DATA - 1u) / CAN_FRAME_MAX_DATA;
    if (numBlocksSz == 0 || numBlocksSz > 64)
    {
        return false;
    }

    const uint8_t numBlocks = static_cast<uint8_t>(numBlocksSz);
    std::vector<std::vector<uint8_t>> blocks(numBlocks, std::vector<uint8_t>(CAN_FRAME_MAX_DATA, 0));

    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        const size_t start = static_cast<size_t>(i) * CAN_FRAME_MAX_DATA;
        const size_t len = std::min<size_t>(CAN_FRAME_MAX_DATA, data.size() - start);
        std::copy_n(data.begin() + start, len, blocks[i].begin());
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        pending.active = true;
        pending.waitingRequestAck = true;
        pending.peer = to;
        pending.numBlocks = numBlocks;
    }

    CanTsHeader req{};
    req.to = to;
    req.from = localAddress;
    req.type = CanTsMessageType::SETBLOCK;
    req.command = C::makeSetBlockCommand(SetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1u));

    if (!frameSender(req, addressLE.data(),
                     static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool gotRequestAck = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                               [&] { return pending.requestAcked || pending.done; });

        if (!gotRequestAck || pending.done || !pending.requestAcked)
        {
            pending = {};
            return false;
        }
    }

    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        CanTsHeader tr{};
        tr.to = to;
        tr.from = localAddress;
        tr.type = CanTsMessageType::SETBLOCK;
        tr.command = C::makeSetBlockCommand(SetBlockFrameType::TRANSFER, i);

        if (!frameSender(tr, blocks[i].data(), CAN_FRAME_MAX_DATA))
        {
            std::lock_guard<std::mutex> lock(mtx);
            pending = {};
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        pending.waitingReport = true;
        pending.reportReceived = false;
        pending.done = false;
        pending.ok = false;
    }

    CanTsHeader statusReq{};
    statusReq.to = to;
    statusReq.from = localAddress;
    statusReq.type = CanTsMessageType::SETBLOCK;
    statusReq.command = C::makeSetBlockCommand(SetBlockFrameType::STATUS_REQUEST, 0);

    if (!frameSender(statusReq, nullptr, 0))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pending = {};
        return false;
    }

    bool success = false;
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool gotReport = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                           [&] { return pending.reportReceived || pending.done; });

        if (!gotReport || !pending.ok || !pending.reportReceived)
        {
            pending = {};
            return false;
        }

        bool allSet = true;
        for (uint8_t i = 0; i < numBlocks; ++i)
        {
            if (!C::bitmapBitIsSet(pending.bitmap, i))
            {
                allSet = false;
                break;
            }
        }

        success = allSet && pending.done;
        pending = {};
    }

    CanTsHeader abort{};
    abort.to = to;
    abort.from = localAddress;
    abort.type = CanTsMessageType::SETBLOCK;
    abort.command = C::makeSetBlockCommand(SetBlockFrameType::ABORT, 0);
    (void)frameSender(abort, nullptr, 0);

    return success;
}
