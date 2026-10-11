import SwiftUI

@main
struct DeskyApp: App {

    @StateObject private var bluetooth = BluetoothManager()

    var body: some Scene {
        MenuBarExtra("Desky", systemImage: "table.furniture") {
            ContentView()
                .environmentObject(bluetooth)
        }
        .menuBarExtraStyle(.window)

        Window("Desky Settings", id: "settings") {
            SettingsView()
                .environmentObject(bluetooth)
                .frame(minWidth: 400, minHeight: 300)
        }
        .windowResizability(.contentSize)
    }
}
