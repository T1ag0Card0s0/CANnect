#pragma once

#include "ICanInterface.hpp"
#include "IRxSink.hpp"
#include "IFrameFilter.hpp"
#include "ICanProtocol.hpp"
#include "Types.hpp"

#include <vector>
#include <memory>
#include <mutex>

class CanRouter : public IRxSink, public ProtocolContext {
public:
    struct RouteEndpoint {
        InterfaceId interfaceId;
        std::shared_ptr<IFrameFilter> filter;
    };

    void registerInterface(ICanInterface& iface) {
        std::scoped_lock lock(mutex_);
        interfaces_.push_back(&iface);
        iface.setReceiveSink(this);
    }

    void registerProtocol(ICanProtocolPtr protocol) {
        std::scoped_lock lock(mutex_);
        protocol->onAttach(*this);
        protocols_.push_back(std::move(protocol));
    }

    void onFrameReceived(const CanFrame& frame) override {
        std::scoped_lock lock(mutex_);

        for (auto& protocol : protocols_) {
            if (protocol->accepts(frame)) {
                protocol->handleFrame(frame);
            }
        }

        routeFrame(frame);
    }

    Status send(const InterfaceId& interfaceId, const CanFrame& frame) override {
        auto* iface = findInterface(interfaceId);
        if (!iface) {
            return {ErrorCode::InvalidArgument, "Interface not found: " + interfaceId};
        }
        return iface->send(frame);
    }

    Status broadcast(const CanFrame& frame) override {
        for (auto* iface : interfaces_) {
            auto status = iface->send(frame);
            if (!status.ok()) {
                return status;
            }
        }
        return Status::Ok();
    }

    Timestamp now() const override {
        return Clock::now();
    }

    void tick() {
        std::scoped_lock lock(mutex_);
        for (auto& protocol : protocols_) {
            protocol->tick();
        }
    }

    void addRoute(RouteEndpoint source, RouteEndpoint destination) {
        std::scoped_lock lock(mutex_);
        routes_.push_back(RouteRule{std::move(source), std::move(destination)});
    }

private:
    struct RouteRule {
        RouteEndpoint source;
        RouteEndpoint destination;
    };

    ICanInterface* findInterface(const InterfaceId& interfaceId) {
        for (auto* iface : interfaces_) {
            if (iface->id() == interfaceId) {
                return iface;
            }
        }
        return nullptr;
    }

    void routeFrame(const CanFrame& frame) {
        for (const auto& route : routes_) {
            if (!frame.sourceInterface.has_value()) {
                continue;
            }

            if (*frame.sourceInterface != route.source.interfaceId) {
                continue;
            }

            if (!route.source.filter || route.source.filter->matches(frame)) {
                auto* dst = findInterface(route.destination.interfaceId);
                if (dst && (!route.destination.filter || route.destination.filter->matches(frame))) {
                    (void)dst->send(frame);
                }
            }
        }
    }

    std::vector<ICanInterface*> interfaces_;
    std::vector<ICanProtocolPtr> protocols_;
    std::vector<RouteRule> routes_;
    mutable std::mutex mutex_;
};

