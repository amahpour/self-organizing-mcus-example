## Startup Sequence Diagram
```mermaid
sequenceDiagram
    participant B1 as Board 1
    participant B2 as Board 2
    participant B3 as Board 3
    participant Bus as Shared Bus

    Note over B1: Powers on
    B1->>Bus: HELLO ("I'm here!")
    Bus-->>B1: No response
    B1->>Bus: CLAIM (nonce=1234)
    Note over B1: Becomes Coordinator (ID #1)

    Note over B2: Powers on
    B2->>Bus: HELLO ("I'm here!")
    B1-->>B2: Coordinator response
    B2->>B1: JOIN (nonce=5678)
    B1-->>B2: ASSIGN (ID #2, nonce=5678)

    Note over B3: Powers on
    B3->>Bus: HELLO ("I'm here!")
    B1-->>B3: Coordinator response
    B3->>B1: JOIN (nonce=9012)
    B1-->>B3: ASSIGN (ID #3, nonce=9012)
```

## Hardware Wiring Diagrams

### Arduino UNO + UNO (SoftwareSerial)
```
    Arduino UNO A                Arduino UNO B
   ┌─────────┐                  ┌─────────┐
   │         │                  │         │
   │   TX(11)├─────────────────►│RX(10)   │
   │         │                  │         │
   │   RX(10)│◄─────────────────┤TX(11)   │
   │         │                  │         │
   │     GND ├──────────────────┤GND      │
   │         │                  │         │
   │    5V/USB                  │5V/USB   │
   └─────────┘                  └─────────┘
        │                           │
     USB Cable                   USB Cable
     (to PC)                     (to PC)
```

### Arduino UNO + R4 WiFi (Mixed Setup)
```
    Arduino UNO                  Arduino R4 WiFi
   ┌─────────┐                  ┌─────────┐
   │         │                  │         │
   │   TX(11)├─────────────────►│RX(0)    │
   │         │                  │         │
   │   RX(10)│◄─────────────────┤TX(1)    │
   │         │                  │         │
   │     GND ├──────────────────┤GND      │
   │         │                  │         │
   │    5V/USB                  │5V/USB   │
   └─────────┘                  └─────────┘
        │                           │
     USB Cable                   USB Cable
     (to PC)                     (to PC)
```

### Arduino R4 WiFi + R4 WiFi (HardwareSerial)
```
    Arduino R4 WiFi A            Arduino R4 WiFi B
   ┌─────────┐                  ┌─────────┐
   │         │                  │         │
   │    TX(1)├─────────────────►│RX(0)    │
   │         │                  │         │
   │    RX(0)│◄─────────────────┤TX(1)    │
   │         │                  │         │
   │     GND ├──────────────────┤GND      │
   │         │                  │         │
   │    5V/USB                  │5V/USB   │
   └─────────┘                  └─────────┘
        │                           │
     USB Cable                   USB Cable
     (to PC)                     (to PC)
```

### Pin Configuration Summary
- **Arduino UNO:** Uses SoftwareSerial on pins 10 (RX) and 11 (TX)
- **Arduino R4 WiFi:** Uses HardwareSerial1 on pins 0 (RX) and 1 (TX)
- **Mixed Setup:** UNO pins 10/11 connect to R4 pins 0/1 respectively