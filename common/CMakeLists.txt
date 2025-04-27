#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#


target_sources(newt_dongle
    PRIVATE
        Endpoint.cpp
        Endpoint.h
        Pipe.cpp
        Pipe.h
        Event.cpp
        Event.h

        Endpoints/UARTEndpoint.cpp
        Endpoints/UARTEndpoint.h
        Endpoints/CDCEndpoint.cpp
        Endpoints/CDCEndpoint.h
)