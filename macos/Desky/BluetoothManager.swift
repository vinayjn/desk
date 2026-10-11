import Combine
import CoreBluetooth

@MainActor
final class BluetoothManager: NSObject, ObservableObject, DeskTransport, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published private(set) var isScanning = false
    @Published private(set) var deskPeripheral: CBPeripheral?
    @Published private(set) var isReady = false
    @Published private(set) var connectionMessage = "Initializing Bluetooth…"
    @Published private(set) var errorMessage: String?

    var readiness: AnyPublisher<Bool, Never> { $isReady.eraseToAnyPublisher() }
    var connectionMessages: AnyPublisher<String, Never> { $connectionMessage.eraseToAnyPublisher() }
    var errors: AnyPublisher<String?, Never> { $errorMessage.eraseToAnyPublisher() }
    var dataCallback: (@MainActor (Data, Bool) -> Void)?

    private var centralManager: CBCentralManager?
    private var characteristics: [CBUUID: CBCharacteristic] = [:]
    private var subscribed = Set<CBUUID>()
    private var receivedHeight = false
    private var receivedStatus = false
    private var shouldReconnect = true
    private var isDisconnecting = false
    private var reconnectTask: Task<Void, Never>?
    private var connectionTimeoutTask: Task<Void, Never>?

    private let serviceID = CBUUID(string: Constants.DESK_SERVICE_ID)
    private let heightID = CBUUID(string: Constants.DESK_GET_HEIGHT_CHAR_ID)
    private let targetID = CBUUID(string: Constants.DESK_SET_HEIGHT_CHAR_ID)
    private let statusID = CBUUID(string: Constants.DESK_GET_STATUS_CHAR_ID)

    override init() {
        super.init()
        // A nil queue delivers Core Bluetooth delegate events on the main queue.
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        guard central.state == .poweredOn else {
            reconnectTask?.cancel()
            connectionTimeoutTask?.cancel()
            isScanning = false
            clearConnection()
            switch central.state {
            case .poweredOff: connectionMessage = "Turn on Bluetooth to connect to the desk."
            case .unauthorized: connectionMessage = "Allow Desky Bluetooth access in System Settings."
            case .unsupported: connectionMessage = "Bluetooth is unavailable on this Mac."
            case .resetting: connectionMessage = "Bluetooth is restarting…"
            default: connectionMessage = "Waiting for Bluetooth…"
            }
            return
        }
        if shouldReconnect { startScanning() }
    }

    func startScanning() {
        shouldReconnect = true
        guard let centralManager, centralManager.state == .poweredOn,
              deskPeripheral == nil, !isScanning else { return }
        reconnectTask?.cancel()
        connectionMessage = "Looking for Standing Desk…"
        isScanning = true
        centralManager.scanForPeripherals(withServices: [serviceID], options: nil)
    }

    private func stopScanning() {
        if centralManager?.state == .poweredOn { centralManager?.stopScan() }
        isScanning = false
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        guard isScanning, deskPeripheral == nil else { return }
        stopScanning()
        deskPeripheral = peripheral
        connectionMessage = "Connecting to \(peripheral.name ?? "Standing Desk")…"
        startConnectionTimeout(for: peripheral)
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        guard peripheral === deskPeripheral, !isDisconnecting else { return }
        peripheral.delegate = self
        resetCharacteristics()
        connectionMessage = "Reading desk controls…"
        peripheral.discoverServices([serviceID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        guard peripheral === deskPeripheral else { return }
        errorMessage = error?.localizedDescription ?? "Could not connect to the desk."
        clearConnection()
        reconnect()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        guard peripheral === deskPeripheral else { return }
        if let error { errorMessage = error.localizedDescription }
        clearConnection()
        reconnect()
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard peripheral === deskPeripheral, !isDisconnecting else { return }
        guard error == nil, let service = peripheral.services?.first(where: { $0.uuid == serviceID }) else {
            failConnection(error?.localizedDescription ?? "The desk Bluetooth service was not found.")
            return
        }
        peripheral.discoverCharacteristics([heightID, targetID, statusID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard peripheral === deskPeripheral, !isDisconnecting, service.uuid == serviceID else { return }
        guard error == nil else {
            failConnection(error?.localizedDescription ?? "Could not read the desk controls.")
            return
        }
        for characteristic in service.characteristics ?? [] {
            characteristics[characteristic.uuid] = characteristic
        }
        guard let height = characteristics[heightID], let status = characteristics[statusID],
              let target = characteristics[targetID], target.properties.contains(.write),
              [height, status].allSatisfy({ $0.properties.contains([.read, .notify]) }) else {
            failConnection("The desk is missing the required Bluetooth controls.")
            return
        }
        for characteristic in [height, status] {
            peripheral.setNotifyValue(true, for: characteristic)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        guard peripheral === deskPeripheral, !isDisconnecting, characteristic === characteristics[characteristic.uuid] else { return }
        guard error == nil, characteristic.isNotifying else {
            failConnection(error?.localizedDescription ?? "Could not subscribe to desk updates.")
            return
        }
        subscribed.insert(characteristic.uuid)
        // Connection notifications can precede discovery: subscribe, then read
        // both values so a stationary desk also supplies an initial snapshot.
        peripheral.readValue(for: characteristic)
        updateReadiness()
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard peripheral === deskPeripheral, !isDisconnecting, characteristic === characteristics[characteristic.uuid] else { return }
        guard error == nil, let data = characteristic.value else {
            failConnection(error?.localizedDescription ?? "Could not read a desk update.")
            return
        }
        if characteristic.uuid == heightID {
            guard DeskProtocol.decodeHeight(data) != nil else {
                failConnection("The desk sent an invalid height.")
                return
            }
            receivedHeight = true
            dataCallback?(data, true)
        } else if characteristic.uuid == statusID {
            guard DeskProtocol.decodeStatus(data) != nil else {
                failConnection("The desk sent an invalid status.")
                return
            }
            receivedStatus = true
            dataCallback?(data, false)
        }
        updateReadiness()
    }

    func peripheral(_ peripheral: CBPeripheral, didModifyServices invalidatedServices: [CBService]) {
        guard peripheral === deskPeripheral, !isDisconnecting, invalidatedServices.contains(where: { $0.uuid == serviceID }) else { return }
        resetCharacteristics()
        connectionMessage = "Refreshing desk controls…"
        startConnectionTimeout(for: peripheral)
        peripheral.discoverServices([serviceID])
    }

    @discardableResult
    func sendHeightData(_ data: Data) -> Bool {
        guard data.count == 1, let byte = data.first, DeskProtocol.heightRange.contains(Int(byte)) else {
            errorMessage = "Choose a height between 72 and 118 cm."
            return false
        }
        guard isReady, let peripheral = deskPeripheral, peripheral.state == .connected,
              let characteristic = characteristics[targetID] else {
            errorMessage = "Wait for the desk to connect before moving it."
            return false
        }
        errorMessage = nil
        peripheral.writeValue(data, for: characteristic, type: .withResponse)
        return true
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        guard peripheral === deskPeripheral, !isDisconnecting, characteristic === characteristics[targetID] else { return }
        if let error {
            errorMessage = "Could not send the height: \(error.localizedDescription)"
        } else if let status = characteristics[statusID] {
            // An already satisfied target can remain IDLE without a notification.
            peripheral.readValue(for: status)
        }
    }

    func disconnect() {
        shouldReconnect = false
        reconnectTask?.cancel()
        stopScanning()
        if let peripheral = deskPeripheral { centralManager?.cancelPeripheralConnection(peripheral) }
        clearConnection()
        connectionMessage = "Disconnected"
    }

    private func updateReadiness() {
        guard !isDisconnecting, !isReady, receivedHeight, receivedStatus,
              subscribed.contains(heightID), subscribed.contains(statusID) else { return }
        connectionTimeoutTask?.cancel()
        errorMessage = nil
        isReady = true
        connectionMessage = "Connected to \(deskPeripheral?.name ?? "Standing Desk")"
    }

    private func resetCharacteristics() {
        isReady = false
        characteristics.removeAll()
        subscribed.removeAll()
        receivedHeight = false
        receivedStatus = false
    }

    private func clearConnection() {
        connectionTimeoutTask?.cancel()
        deskPeripheral?.delegate = nil
        deskPeripheral = nil
        isDisconnecting = false
        resetCharacteristics()
    }

    private func failConnection(_ message: String) {
        connectionTimeoutTask?.cancel()
        isDisconnecting = true
        errorMessage = message
        connectionMessage = message
        isReady = false
        if let peripheral = deskPeripheral { centralManager?.cancelPeripheralConnection(peripheral) }
        // Retain the peripheral until cancellation completes, avoiding a second
        // connection being started while the old one is still disconnecting.
    }

    private func reconnect() {
        connectionMessage = "Desk disconnected. Reconnecting…"
        guard shouldReconnect else { return }
        reconnectTask?.cancel()
        reconnectTask = Task { [weak self] in
            do { try await Task.sleep(for: .seconds(2)) } catch { return }
            self?.startScanning()
        }
    }

    private func startConnectionTimeout(for peripheral: CBPeripheral) {
        connectionTimeoutTask?.cancel()
        let identifier = peripheral.identifier
        connectionTimeoutTask = Task { [weak self] in
            do { try await Task.sleep(for: .seconds(20)) } catch { return }
            guard let self, self.deskPeripheral?.identifier == identifier, !self.isReady else { return }
            self.failConnection("The desk connection timed out. Retrying…")
        }
    }
}
