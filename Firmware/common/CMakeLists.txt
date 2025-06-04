#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#


target_sources(${APP} PRIVATE
        Endpoint.cpp
        Endpoint.h
        Event.cpp
        Event.h
        Logger.cpp
        Logger.h
        Pipe.cpp
        Pipe.h
        Scheduler.cpp
        Scheduler.h
        Task.cpp
        Task.h
        UserSettings.cpp
        UserSettings.h

        Endpoints/Dock.cpp
        Endpoints/Dock.h
        Endpoints/SDCardEndpoint.cpp
        Endpoints/SDCardEndpoint.h
        Endpoints/StdioLog.cpp
        Endpoints/StdioLog.h
        Endpoints/TestEventGenerator.cpp
        Endpoints/TestEventGenerator.h
        Endpoints/UARTEndpoint.cpp
        Endpoints/UARTEndpoint.h

        Filters/HayesFilter.cpp
        Filters/HayesFilter.h
        Filters/MNPFilter.cpp
        Filters/MNPFilter.h

        Newton/NSOF.cpp
        Newton/NSOF.h

        Pipes/BufferedPipe.cpp
        Pipes/BufferedPipe.h
        Pipes/MNPThrottle.cpp
        Pipes/MNPThrottle.h
        Pipes/Tee.cpp
        Pipes/Tee.h
)