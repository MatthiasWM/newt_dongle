

target_sources(newt_dongle
    PRIVATE
        Buffer.cpp
        Buffer.h
        CtrlBlock.cpp
        CtrlBlock.h
        CtrlBuffer.cpp
        CtrlBuffer.h
        DataBuffer.cpp
        DataBuffer.h
        Endpoint.cpp
        Endpoint.h
        Pipe.cpp
        Pipe.h

        Endpoints/UARTEndpoint.cpp
        Endpoints/UARTEndpoint.h
        Endpoints/CDCEndpoint.cpp
        Endpoints/CDCEndpoint.h
)