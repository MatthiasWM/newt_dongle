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
        if ((dtr_switch_.dtr_set==true) && dtr_switch_.out()) {
            return dtr_switch_.out()->send(event);
        } else {
            return Result::OK__NOT_CONNECTED;
        }
    }
    Result rush(Event event) override  {
        if ((event.type() == Event::Type::UART) && (event.subtype() == Event::Subtype::UART_DTR)) {
            Log.logf("DTRSwitch: DTR set to %d\n", event.data());
            dtr_switch_.dtr_set = (event.data() != 0);
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
