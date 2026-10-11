import Combine

/// Owned by the App, so closing the menu does not end the Bluetooth connection.
@MainActor
final class DeskyModel: ObservableObject {
    let bluetoothManager: BluetoothManager
    let contentViewModel: ContentViewModel

    init() {
        let manager = BluetoothManager()
        bluetoothManager = manager
        contentViewModel = ContentViewModel(bluetoothManager: manager)
    }
}
