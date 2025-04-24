
#ifndef ND_ENDPOINTS_UART_H
#define ND_ENDPOINTS_UART_H

#include "../Endpoint.h" // Adjusted the path to ensure the Endpoint header is correctly included

namespace nd {

class UARTEndpoint : public Endpoint {
public:
    UARTEndpoint();
    ~UARTEndpoint();

    int init() override;
    int task() override;
};

} // namespace nd

#endif // ND_ENDPOINTS_UART_H