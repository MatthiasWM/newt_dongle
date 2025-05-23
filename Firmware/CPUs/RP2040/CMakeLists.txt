#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#

target_sources(newt_dongle
    PRIVATE
        PicoAsyncLog.cpp
        PicoAsyncLog.h
        PicoCDCEndpoint.cpp
        PicoCDCEndpoint.h
        PicoUARTEndpoint.cpp
        PicoUARTEndpoint.h
        PicoScheduler.cpp
        PicoScheduler.h
        PicoStdioLog.cpp
        PicoStdioLog.h
        PicoSDCard.cpp
        PicoSDCard.h
        PicoSystem.cpp
        PicoSystem.h
        PicoUserSettings.cpp
        PicoUserSettings.h

        PicoDock.cpp
        PicoDock.h
)