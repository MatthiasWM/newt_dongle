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
        PicoSDCard.cpp
        PicoSDCard.h
        PicoStatusDisplay.cpp
        PicoStatusDisplay.h
        PicoSystemTask.cpp
        PicoSystemTask.h
        PicoUserSettings.cpp
        PicoUserSettings.h
)