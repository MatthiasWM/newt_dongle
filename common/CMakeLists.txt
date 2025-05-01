#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#


target_sources(newt_dongle
    PRIVATE
        Endpoint.cpp
        Endpoint.h
        Event.cpp
        Event.h
        Pipe.cpp
        Pipe.h
        Task.cpp
        Task.h
        Scheduler.cpp
        Scheduler.h

        Endpoints/SDCardEndpoint.cpp
        Endpoints/SDCardEndpoint.h
        Endpoints/StdioLog.cpp
        Endpoints/StdioLog.h
        Endpoints/TestEventGenerator.cpp
        Endpoints/TestEventGenerator.h
        Endpoints/UARTEndpoint.cpp
        Endpoints/UARTEndpoint.h

        Pipes/BufferedPipe.cpp
        Pipes/BufferedPipe.h
        Pipes/Tee.cpp
        Pipes/Tee.h

)