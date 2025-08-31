# Quick Demo Steps

## The Process
1. AI discovers connected Arduino boards
2. AI uploads empty sketches ("power off" simulation)  
3. AI uploads AutoSort to first board → becomes coordinator
4. AI demonstrates role switching with timing

## What You See
```
AutoSort Arduino starting...
Node[0] CLAIM nonce=1234567890
Node[0] → COORDINATOR (ID=1)
```

## Key Insights
- **Same code, different timing = different roles** - No hardcoded differences needed
- **Coordinator stability** - Once elected, coordinator never steps down to challengers
- **Mixed board support** - Works with Arduino UNO (SoftwareSerial) and UNO R4 (HardwareSerial)
