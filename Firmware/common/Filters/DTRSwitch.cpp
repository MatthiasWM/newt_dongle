//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "DTRSwitch.h"

#include "main.h"

using namespace nd;

namespace nd {

class ToDockPipe : public Pipe {
    DTRSwitch &dtr_switch_;
public:
    ToDockPipe(DTRSwitch &dtr_switch) : dtr_switch_(dtr_switch) { }
    Result send(Event event) override {
        if ((dtr_switch_.dtr_set==false) && dtr_switch_.out()) {
            Result r = dtr_switch_.send_pending_bitrate_();
            if (r.rejected()) return r;
            return dtr_switch_.out()->send(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
    Result rush(Event event) override {
        if ((dtr_switch_.dtr_set==false) && dtr_switch_.out()) {
            return dtr_switch_.out()->rush(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
    Result rush_back(Event event) override {
        if ((dtr_switch_.dtr_set==false) && dtr_switch_.in()) {
            return dtr_switch_.in()->rush_back(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
};

class ToCDCPipe : public Pipe {
    DTRSwitch &dtr_switch_;
public:
    ToCDCPipe(DTRSwitch &dtr_switch) : dtr_switch_(dtr_switch) { }
    Result send(Event event) override {
        if (event.type() == Event::Type::SET_BITRATE) {
            // Remember the host's bitrate. The UART uses it in direct connect mode only.
            dtr_switch_.host_bitrate_ = event.bitrate();
            if (dtr_switch_.dtr_set) {
                dtr_switch_.pending_bitrate_ = dtr_switch_.host_bitrate_;
                dtr_switch_.send_pending_bitrate_(); // if rejected, it goes out before the next byte
            }
            return Result::OK;
        }
        if ((dtr_switch_.dtr_set==true) && dtr_switch_.out()) {
            Result r = dtr_switch_.send_pending_bitrate_();
            if (r.rejected()) return r;
            return dtr_switch_.out()->send(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
    Result rush(Event event) override  {
        if ((event.type() == Event::Type::UART) && (event.subtype() == Event::Subtype::UART_DTR)) {
            bool dtr = (event.data() != 0);
            if (dtr != dtr_switch_.dtr_set) {
                // Switching modes ends the MNP session on the Dock side...
                if (dtr_switch_.dock_->out())
                    dtr_switch_.dock_->out()->rush(event);
                // ...a full buffer on the old side must not throttle the Newton...
                if (dtr_switch_.in())
                    dtr_switch_.in()->rush_back(Event(Event::Type::HIGH_WATER, Event::Subtype::RESET));
                // ...and the UART uses the host's bitrate, or the Dock's in Dock mode.
                dtr_switch_.pending_bitrate_ = dtr ? dtr_switch_.host_bitrate_ : kUART_BaudRate;
                dtr_switch_.send_pending_bitrate_();
            }
            if (event.data() == 0) {
                dtr_switch_.dtr_set = false;
                app_status.set(AppStatus::IDLE);
                if (kLogDTRSwitch) Log.log("DTRSwitch: DTR set to false\n");
            } else {
                dtr_switch_.dtr_set = true;
                app_status.set(AppStatus::USB_CONNECTED);
                if (kLogDTRSwitch) Log.log("DTRSwitch: DTR set to true\n");
            }
        }
        if ((dtr_switch_.dtr_set==true) && dtr_switch_.out()) {
            return dtr_switch_.out()->rush(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
    Result rush_back(Event event) override {
        if ((dtr_switch_.dtr_set==true) && dtr_switch_.in()) {
            return dtr_switch_.in()->rush_back(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
};

}; // namespace nd


DTRSwitch::DTRSwitch() 
:   super(),
	dock_(new ToDockPipe(*this)),
	cdc_(new ToCDCPipe(*this)),
	dock(*dock_),
    cdc(*cdc_)
{
}

/**
 * \brief Release all resources.
 * This will delete the dock and newt pipes, and all frames in the pool.
 */
DTRSwitch::~DTRSwitch() 
{
    delete dock_;
    delete cdc_;
}

/**
 * \brief Send a bitrate change towards the UART that was not accepted yet.
 * \return REJECTED if the UART side is still busy. The caller must then hold
 *         back its data, so it isn't sent at the old bitrate.
 */
Result DTRSwitch::send_pending_bitrate_()
{
    if ((pending_bitrate_ == 0) || !out())
        return Result::OK;
    Result r = out()->send(Event::make_bitrate_event(pending_bitrate_));
    if (!r.rejected())
        pending_bitrate_ = 0;
    return r;
}

Result DTRSwitch::send(Event event)
{
    if (dtr_set) {
        if (cdc_ && cdc_->out()) {
            return cdc_->out()->send(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    } else {
        if (dock_ && dock_->out()) {
            return dock_->out()->send(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
}

Result DTRSwitch::rush(Event event)
{
    if (dtr_set) {
        if (cdc_ && cdc_->out()) {
            return cdc_->out()->rush(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    } else {
        if (dock_ && dock_->out()) {
            return dock_->out()->rush(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
}

Result DTRSwitch::rush_back(Event event)
{
    if (dtr_set) {
        if (cdc_ && cdc_->in()) {
            return cdc_->in()->rush_back(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    } else {
        if (dock_ && dock_->in()) {
            return dock_->in()->rush_back(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
}
