import Combine
import CoreBluetooth

class BluetoothManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published var centralManager: CBCentralManager!
    @Published var isScanning = false
    @Published var discoveredDevices: [CBPeripheral] = []
    @Published var connectedDevice: CBPeripheral?
    @Published var connectionStatus = "Initializing..."
    @Published var receivedData = ""

    @Published var serviceCharacteristics = [CBService: [CBCharacteristic]]()
    @Published var selectedService: CBService?
    @Published var selectedCharacteristic: CBCharacteristic?

    var services: [CBService] {
        serviceCharacteristics.keys.sorted { $0.uuid.uuidString < $1.uuid.uuidString }
    }

    var characteristics: [CBCharacteristic] {
        if let selectedService {
            return serviceCharacteristics[selectedService] ?? []
        }
        return []
    }

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    func encodeHeight(_ heightInCm: Int) -> Data? {
        guard heightInCm >= 72 && heightInCm <= 118 else {
            print("Error: Height \(heightInCm) is out of valid range (72-118 cm)")
            return nil
        }

        let byte = UInt8(heightInCm)
        return Data([byte])
    }

    func decodeHeight(from data: Data) -> Int? {
        guard let byte = data.first else {
            print("Error: No data received")
            return nil
        }

        let height = Int(byte)

        guard height >= 72 && height <= 118 else {
            print("Warning: Received height \(height) is out of expected range")
            return nil
        }

        return height
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        DispatchQueue.main.async {
            switch central.state {
            case .poweredOn:
                self.connectionStatus = "Ready to scan"
            case .poweredOff:
                self.connectionStatus = "Turn on Bluetooth in Settings"
            case .unauthorized:
                self.connectionStatus = "Allow Bluetooth access in Settings"
            case .unsupported:
                self.connectionStatus = "Device doesn't support Bluetooth"
            case .resetting:
                self.connectionStatus = "Bluetooth is resetting..."
            case .unknown:
                self.connectionStatus = "Bluetooth state unknown"
            @unknown default:
                self.connectionStatus = "Unknown Bluetooth state"
            }

            print("Bluetooth state changed to: \(central.state.rawValue)")
        }
    }

    func startScanning() {
        guard centralManager.state == .poweredOn else {
            connectionStatus = "Bluetooth not ready"
            return
        }

        discoveredDevices.removeAll()
        isScanning = true
        connectionStatus = "Scanning..."

        centralManager.scanForPeripherals(withServices: nil, options: [
            CBCentralManagerScanOptionSolicitedServiceUUIDsKey: []
        ])

        DispatchQueue.main.asyncAfter(deadline: .now() + 30) {
            if self.isScanning {
                self.stopScanning()
            }
        }
    }

    func stopScanning() {
        centralManager.stopScan()
        isScanning = false
        let deviceCount = discoveredDevices.count
        connectionStatus = deviceCount == 0 ? "No devices found" : "Found \(deviceCount) device(s)"
        print("Scan stopped. Total devices found: \(deviceCount)")
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if !discoveredDevices.contains(where: { $0.identifier == peripheral.identifier }) {
            DispatchQueue.main.async {
                self.discoveredDevices.append(peripheral)
                if self.isScanning {
                    self.connectionStatus = "Found \(self.discoveredDevices.count) device(s)..."
                }
            }
        }
    }

    func connect(to peripheral: CBPeripheral) {
        centralManager.connect(peripheral, options: nil)
        connectionStatus = "Connecting..."
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectedDevice = peripheral
        peripheral.delegate = self
        connectionStatus = "Connected to \(peripheral.name ?? "Unknown")"
        self.stopScanning()

        serviceCharacteristics = [:]
        peripheral.discoverServices(nil)
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        connectionStatus = "Failed to connect"
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else {
            return
        }

        print("Discovered \(services.count) services for \(peripheral.name ?? "Unknown"):")
        for service in services {
            serviceCharacteristics[service] = []
            print("  Service UUID: \(service.uuid)")
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else {
            print("No characteristics found for service \(service.uuid)")
            return
        }

        print("Discovered \(characteristics.count) characteristics for service \(service.uuid):")

        serviceCharacteristics[service] = characteristics

        for characteristic in characteristics {
            if characteristic.properties.contains(.notify) {
                peripheral.setNotifyValue(true, for: characteristic)
            }
        }

        DispatchQueue.main.async {
            self.connectionStatus = "Connected"
            self.selectedService = service
            self.selectedCharacteristic = characteristics.first
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let data = characteristic.value {
            if let height = decodeHeight(from: data) {
                DispatchQueue.main.async {
                    self.receivedData += "\(height)"
                }
            }
        }
    }

    func sendCommand(_ command: String) {
        guard let device = connectedDevice,
              let height = Int(command),
              let characteristic = selectedCharacteristic,
              let encodedData = encodeHeight(height) else {
            receivedData += "Error: Cannot send \(command) - not connected\n"
            return
        }

        Task {
            device.writeValue(encodedData, for: characteristic, type: .withResponse)
            receivedData += "Sent: \(command)\n"
        }
    }

    func disconnect() {
        if let device = connectedDevice {
            centralManager.cancelPeripheralConnection(device)
            connectedDevice = nil
            connectionStatus = "Disconnected"
        }
    }
}
