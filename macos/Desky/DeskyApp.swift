import SwiftUI

@main
struct DeskyApp: App {
    @StateObject private var model = DeskyModel()

    var body: some Scene {
        MenuBarExtra("Desky", systemImage: "table.furniture") {
            ContentView(viewModel: model.contentViewModel)
        }
        .menuBarExtraStyle(.window)

        Window("Desky Settings", id: "settings") {
            SettingsView()
                .environmentObject(model.bluetoothManager)
                .frame(minWidth: 400, minHeight: 300)
        }
        .windowResizability(.contentSize)
    }
}
