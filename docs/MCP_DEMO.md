# AI Agent Controls Arduino Hardware

An AI agent (Claude) directly controlled physical Arduino boards through MCP tools to demonstrate self-organizing systems.

## What Happened

**Human:** "Test this self-organizing Arduino system"  
**AI:** *Discovers boards, compiles code, uploads sketches, monitors output*  
**Result:** AI successfully orchestrated a hardware demo showing coordinator election

## The Demo

**Step 1:** AI discovers two connected Arduino boards  
**Step 2:** AI uploads empty sketches to simulate "powered off"  
**Step 3:** AI "powers on" first board with AutoSort sketch  
**Step 4:** Board becomes coordinator: `Node[0] → COORDINATOR (ID=1)`  
**Step 5:** AI demonstrates role switching with different power timing  

## Key Point

Same identical code on both boards - timing determines who becomes coordinator, not hardcoded differences.

## Why This Matters

AI agents can now directly control physical hardware through MCP - no human copy/paste needed. This enables new patterns of human-AI collaboration in embedded development.

## Setup

- Install [Arduino MCP Server](https://github.com/amahpour/arduino-mcp-server-simple)  
- Connect Arduino boards via USB
- Use MCP-compatible AI client

The AI handles compilation, uploading, and monitoring automatically.
