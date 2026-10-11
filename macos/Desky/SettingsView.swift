import Combine
import SwiftUI
import CoreBluetooth

struct SettingsView: View {
    @EnvironmentObject var bluetoothManager: BluetoothManager

    var body: some View {
        Form {
            Section("Bluetooth") {
                Text(bluetoothManager.connectionMessage)
                if let error = bluetoothManager.errorMessage {
                    Text(error).foregroundStyle(.red)
                }
            }
            if let device = bluetoothManager.deskPeripheral {
                Section("Your Desks") {
                    HStack {
                        Text(device.name ?? "Unknown Device")
                        Spacer()

                    }
                }
            } else {
                VStack {
                    HStack {
                        Button {
                            bluetoothManager.startScanning()
                        } label: {
                            Text("Start Scanning")
                        }
                        .disabled(bluetoothManager.isScanning)
                    }
                }
            }
        }
        .formStyle(.grouped)
    }
}

#Preview {
    SettingsView()
        .environmentObject(BluetoothManager())
}
