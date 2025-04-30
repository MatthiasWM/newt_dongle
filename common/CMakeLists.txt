#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#


target_sources(newt_dongle
    PRIVATE
        Device.cpp
        Device.h
        Event.cpp
        Event.h
        Pipe.cpp
        Pipe.h
        Wheel.cpp
        Wheel.h

        Devices/StdioLog.cpp
        Devices/StdioLog.h

        Pipes/BufferedPipe.cpp
        Pipes/BufferedPipe.h
        Pipes/Tee.cpp
        Pipes/Tee.h

#        Endpoints/UARTEndpoint.cpp
#        Endpoints/UARTEndpoint.h
#        Endpoints/CDCEndpoint.cpp
#        Endpoints/CDCEndpoint.h
)