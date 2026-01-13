# CS2 Match Tracker

Automatically posts roasts and commentary to Discord when you finish a CS2 match.

## What it does

This thing watches your Leetify profile for new matches. When you finish a game, it grabs all the stats (yours, your teammates, enemies) and sends them to an AI that writes a funny comment about how you did. Then it posts that to your Discord server.

If you got destroyed by some guy who went 30-5, it'll probably mention that. If you bottom fragged, you're getting roasted. If you actually popped off, maybe you'll get some praise. The AI sees everything.

## Setup

You'll need a few API keys (all free):

1. **Leetify API key** - Go to leetify.com, Settings > API Access
2. **Discord webhook** - Server Settings > Integrations > Webhooks, create one for whatever channel you want
3. **Groq API key** - console.groq.com/keys (free, uses Llama 3.1)

Also grab your Steam64 ID from steamid.io

## Building

Needs OpenSSL and a C++17 compiler.

```
mkdir build
cd build
cmake ..
make
```

## Config

Create a `.env` file in the build folder:

```
DISCORD_WEBHOOK_URL=https://discord.com/api/webhooks/xxxxx/xxxxx
LEETIFY_API_KEY=your_leetify_key
GROQ_API_KEY=gsk_xxxxx
TRACKED_STEAM_IDS=76561198012345678,76561198087654321
POLL_INTERVAL_SECONDS=60
```

You can track multiple people, just comma-separate the Steam IDs.

## Running

```
./CounterStrikeHelper
```

It'll poll every 60 seconds (or whatever you set) and post when it finds a new match. Ctrl+C to stop.

It remembers which matches it's already posted about in `seen_matches.txt`, so you won't get spammed if you restart it.

## Dependencies

- httplib (header-only, stick it in include/)
- nlohmann/json (header-only, stick it in include/)
- OpenSSL

## File structure

```
├── include/
│   ├── ai_client.h
│   ├── config.h
│   ├── discord_client.h
│   ├── leetify_client.h
│   ├── match_data.h
│   ├── persistence.h
│   ├── httplib.h
│   └── nlohmann/json.hpp
├── src/
│   ├── ai_client.cpp
│   ├── config.cpp
│   ├── discord_client.cpp
│   ├── leetify_client.cpp
│   └── match_data.cpp
├── main.cpp
├── CMakeLists.txt
└── .env
```

## Notes

- Leetify needs to be tracking your matches for this to work (just link your Steam account on their site)
- The free Groq tier is pretty generous, you won't hit limits unless you're playing like 100 games a day
- If the AI says something too savage, that's on Llama not me