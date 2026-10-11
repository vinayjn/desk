# Desky

A macOS menu bar app for controlling a standing desk over Bluetooth using the
[ESP32 desk firmware](https://github.com/vinayjn/desk).

Choose a height with the slider or the sitting and standing presets. The app
shows the desk's reported height, keeps its connection when the menu closes,
and reconnects automatically. Controls wait for the desk to acknowledge each
move; errors allow a new height to retry. Physical keypad controls take priority.

## Run

Open `Desky.xcodeproj` in Xcode and run the **Desky** scheme on macOS 26 or later.
Allow Bluetooth access when prompted. Connection details are in Settings.

## Verify

```sh
bash scripts/test.sh
xcodebuild -project Desky.xcodeproj -scheme Desky -configuration Release \
  -derivedDataPath /tmp/Desky-build CODE_SIGNING_ALLOWED=NO build
```

Tests use a simulated desk transport. Bluetooth discovery and movement require
the desk hardware. Protocol UUIDs and height limits are in `DeskProtocol.swift`.
