import Combine
import SwiftUI
import CoreBluetooth

struct SettingsView: View {
    @EnvironmentObject var bluetoothManager: BluetoothManager

    @State private var hoveredId: UUID?

    var body: some View {
        Form {
            if let device = bluetoothManager.connectedDevice {
                Section("Your Desks") {
                    HStack {
                        Text(device.name ?? "Unknown Device")
                        Spacer()

                    }
                }
            } else {
                VStack {
                    HStack {
                        Text(bluetoothManager.connectionStatus)
                        Spacer()
                        Button {
                            bluetoothManager.startScanning()
                        } label: {
                            Text("Start Scanning")
                        }
                        .disabled(bluetoothManager.isScanning)
                    }
                }
            }

            if bluetoothManager.connectedDevice == nil, bluetoothManager.discoveredDevices.count > 0 {
                Section("Discovered Devices") {
                    ForEach(bluetoothManager.discoveredDevices, id: \.identifier) { device in
                        HStack {
                            Text(device.name ?? "Unknwon name")

                            Spacer()

                            if hoveredId == device.identifier {
                                Button {
                                    bluetoothManager.connect(to: device)
                                } label: {
                                    Text("Connect")
                                }
                            }

                        }
                        .onHover { hovering in
                            if hovering {
                                hoveredId = device.identifier
                            } else {
                                hoveredId = nil
                            }
                        }
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
