#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#

target_sources(${APP} PRIVATE

        PosixScheduler.cpp
        PosixScheduler.h
        PosixSystemTask.cpp
        PosixSystemTask.h

        Endpoints/PosixSDCardEndpoint.cpp
        Endpoints/PosixSDCardEndpoint.h
)