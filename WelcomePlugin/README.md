# Welcome Plugin

A simple example plugin for ARK: Survival Ascended that demonstrates basic plugin development patterns.

## Features

- **Welcome Messages**: Automatically greets players when they join the server
- **Server Information Command**: Players can use `/serverinfo` to view server details
- **Configuration Reload**: RCON command to reload configuration without restarting
- **Customizable Messages**: Fully configurable welcome messages and server information

## Installation

1. Build the plugin using Visual Studio 2022
2. Copy the compiled DLL to your server's `AsaApi/Plugins/WelcomePlugin/` folder
3. Copy the `Configs` folder to `AsaApi/Plugins/WelcomePlugin/`
4. Restart your server

## Configuration

Edit `config.json` in the `AsaApi/Plugins/WelcomePlugin/` folder:

```json
{
  "General": {
    "ServerName": "My ASA Server",
    "WelcomeMessage": "Welcome {player} to {server}!",
    "WelcomeEnabled": true,
    "TextSize": 1.5,
    "DisplayTime": 5.0
  },
  "ServerInfo": {
    "Description": "A great Ark: Survival Ascended server!",
    "Website": "www.example.com",
    "Discord": "discord.gg/example"
  }
}
```

### Configuration Options

- `ServerName`: The name of your server (used in welcome messages)
- `WelcomeMessage`: Message shown to players on join. Use `{player}` for player name and `{server}` for server name
- `WelcomeEnabled`: Enable/disable welcome messages
- `TextSize`: Size of notification text (default: 1.5)
- `DisplayTime`: How long notifications display in seconds (default: 5.0)
- `ServerInfo`: Information shown when players use `/serverinfo` command

## Commands

### Chat Commands
- `/serverinfo` - Displays server information including description, website, and Discord

### RCON Commands
- `welcomeplugin.reload` - Reloads the plugin configuration

## Building

### Requirements
- Visual Studio 2022
- ARK Server API (AsaApi)
- C++20 compiler

### Build Steps
1. Open `WelcomePlugin.sln` in Visual Studio 2022
2. Ensure the AsaApi path is correct in the project settings
3. Build in Release x64 configuration
4. The compiled DLL will be in `x64/Release/`

## Development Notes

This plugin demonstrates:
- Basic plugin structure and entry points
- Hook system for game events
- Chat and RCON command registration
- Configuration file handling with JSON
- Player notification system
- Logging and error handling

## Version

**Version:** 1.0
**Min API Version:** 1.19

## License

This is an example plugin provided as-is for educational purposes.
