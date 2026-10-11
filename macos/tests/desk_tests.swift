import Combine
import Foundation

@MainActor
final class FakeDeskTransport: DeskTransport {
    let ready = CurrentValueSubject<Bool, Never>(false)
    let messages = CurrentValueSubject<String, Never>("Connecting…")
    let failures = CurrentValueSubject<String?, Never>(nil)
    var readiness: AnyPublisher<Bool, Never> { ready.eraseToAnyPublisher() }
    var connectionMessages: AnyPublisher<String, Never> { messages.eraseToAnyPublisher() }
    var errors: AnyPublisher<String?, Never> { failures.eraseToAnyPublisher() }
    var dataCallback: (@MainActor (Data, Bool) -> Void)?
    var writes: [Data] = []
    var rejectWrites = false
    var scans = 0

    func startScanning() { scans += 1 }

    func sendHeightData(_ data: Data) -> Bool {
        guard ready.value, !rejectWrites else {
            failures.send("Write failed")
            return false
        }
        failures.send(nil)
        writes.append(data)
        return true
    }

    func height(_ byte: UInt8) { dataCallback?(Data([byte]), true) }
    func status(_ value: String) { dataCallback?(Data(value.utf8), false) }
    func connect(height: UInt8 = 85, status: String = "IDLE") {
        self.height(height)
        self.status(status)
        ready.send(true)
    }
}

@main
@MainActor
struct DeskTests {
    static func expect(_ condition: @autoclosure () -> Bool, _ message: String) {
        precondition(condition(), message)
    }

    static func main() async throws {
        expect(DeskProtocol.encodeHeight(100) == Data([0x64]), "Target must be one binary byte")
        expect(DeskProtocol.encodeHeight(99.8) == Data([100]), "Fractional targets round consistently")
        for height in [Double.nan, .infinity, -.infinity, 71, 119] {
            expect(DeskProtocol.encodeHeight(height) == nil, "Invalid target must not be encoded")
        }
        for height in [72, 118] {
            expect(DeskProtocol.encodeHeight(Double(height)) == Data([UInt8(height)]), "Valid height boundary")
        }
        expect(DeskProtocol.decodeHeight(Data([0])) == .unknown, "Zero is unknown height")
        expect(DeskProtocol.decodeHeight(Data([85])) == .centimeters(85), "Binary height decoding")
        for data in [Data(), Data([71]), Data([119]), Data([85, 0]), Data("100".utf8)] {
            expect(DeskProtocol.decodeHeight(data) == nil, "Malformed height must be rejected")
        }
        expect(DeskProtocol.decodeStatus(Data("IDLE".utf8)) == .idle, "IDLE read")
        expect(DeskProtocol.decodeStatus(Data("MOVING".utf8)) == .moving, "MOVING notification")
        expect(DeskProtocol.decodeStatus(Data("ERROR".utf8)) == .error, "ERROR notification")
        expect(DeskProtocol.decodeStatus(Data([1])) == nil, "Old numeric status is not rewritten protocol")
        expect(DeskProtocol.decodeStatus(Data("INVALID".utf8)) == nil, "Reject malformed status")

        let desk = FakeDeskTransport()
        let model = ContentViewModel(bluetoothManager: desk)
        expect(!model.canMove && model.currentHeight == nil, "Disable movement until setup is ready")
        model.onDragEnded(value: 100)
        expect(desk.writes.isEmpty, "Disconnected commands must not be sent")
        desk.connect()
        expect(model.isConnected && model.canMove, "Initial snapshot enables controls")
        expect(model.currentHeight == 85 && model.selectedHeight == 85, "Initial read updates height")

        model.sliderEditingChanged(true)
        model.selectedHeight = 100
        desk.height(90)
        expect(model.currentHeight == 90 && model.selectedHeight == 100, "Feedback must not overwrite a drag")
        model.sliderEditingChanged(false)
        expect(desk.writes.last == Data([100]), "Send the chosen target, not incoming feedback")
        expect(model.currentHeight == 90, "Command must not falsify reported height")
        expect(model.isAwaitingResponse && !model.canMove, "Prevent duplicate writes while awaiting status")
        model.setPreset(.standing)
        expect(desk.writes.count == 1, "Ignore new target until desk responds")
        desk.status("IDLE")
        expect(model.isAwaitingResponse, "An old IDLE read must not acknowledge an unmet target")
        desk.status("MOVING")
        expect(!model.isAwaitingResponse && !model.canMove, "Moving status completes acknowledgement and locks controls")
        desk.height(98)
        expect(model.selectedHeight == 100, "Keep target while moving")
        desk.height(100)
        desk.status("IDLE")
        expect(model.canMove && model.currentHeight == 100, "Final IDLE restores controls")

        model.onDragEnded(value: 100)
        desk.status("IDLE")
        expect(!model.isAwaitingResponse && model.canMove, "Equal-height target can acknowledge with IDLE")
        model.setPreset(.standing)
        expect(model.currentHeight == 100 && desk.writes.last == Data([110]), "Preset is a target, not measured height")
        desk.status("ERROR")
        expect(model.deskState == .error && model.canMove, "Persistent ERROR must allow a retry")
        model.setPreset(.standing)
        expect(model.isAwaitingResponse, "Retry from ERROR sends a new command")
        desk.status("MOVING")
        desk.height(110)
        desk.status("IDLE")

        desk.rejectWrites = true
        model.setPreset(.sitting)
        expect(!model.isAwaitingResponse && model.errorMessage == "Write failed", "Failed write must be visible and release controls")
        expect(model.currentHeight == 110 && model.selectedHeight == 110, "Failed write keeps actual height")
        desk.rejectWrites = false
        model.setPreset(.sitting)
        desk.ready.send(false)
        expect(!model.isConnected && model.currentHeight == nil && !model.isAwaitingResponse, "Disconnect clears stale data and pending command")
        desk.connect(height: 0)
        expect(model.currentHeight == nil && model.canMove, "Unknown height must remain unknown and permit a fresh query via target")
        desk.height(80)
        model.onDragEnded(value: 81)
        try await Task.sleep(for: .milliseconds(3200))
        expect(!model.isAwaitingResponse && model.canMove && model.errorMessage != nil, "Missing status must not permanently lock controls")
        model.startScanning()
        expect(desk.scans == 1, "Scanning action reaches transport")

        weak var releasedModel: ContentViewModel?
        do {
            let temporaryModel = ContentViewModel(bluetoothManager: FakeDeskTransport())
            releasedModel = temporaryModel
        }
        expect(releasedModel == nil, "Subscriptions and data callback must not retain the model")
        print("Desk protocol, initial state, target selection, movement, error recovery and disconnect tests passed")
    }
}
