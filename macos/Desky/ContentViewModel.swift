import Combine
import Foundation

@MainActor
final class ContentViewModel: ObservableObject {
    @Published private(set) var deskState: DeskStatus = .idle
    @Published private(set) var currentHeight: Int?
    @Published var selectedHeight: Double = 72
    @Published private(set) var isSliderDragging = false
    @Published private(set) var isConnected = false
    @Published private(set) var isAwaitingResponse = false
    @Published private(set) var errorMessage: String?
    @Published var selectedAutomation: Automation?

    let range = Double(DeskProtocol.heightRange.lowerBound)...Double(DeskProtocol.heightRange.upperBound)
    var canMove: Bool { isConnected && deskState != .moving && !isAwaitingResponse }

    var displayedErrorMessage: String? {
        if let errorMessage { return errorMessage }
        return deskState == .error ? "Desk stopped with an error. Choose a height to retry." : nil
    }

    private let bluetoothManager: any DeskTransport
    private var cancellables = Set<AnyCancellable>()
    private var responseTimeoutTask: Task<Void, Never>?

    init(bluetoothManager: any DeskTransport) {
        self.bluetoothManager = bluetoothManager
        bluetoothManager.readiness
            .removeDuplicates()
            .sink { [weak self] ready in
                guard let self else { return }
                self.isConnected = ready
                if !ready {
                    self.finishWaiting()
                    self.currentHeight = nil
                    self.deskState = .idle
                    self.isSliderDragging = false
                }
            }.store(in: &cancellables)
        bluetoothManager.errors
            .sink { [weak self] error in
                self?.errorMessage = error
                if error != nil { self?.finishWaiting() }
            }.store(in: &cancellables)
        bluetoothManager.dataCallback = { [weak self] data, isHeight in
            self?.didReceiveData(data, isHeight: isHeight)
        }
    }

    func didReceiveData(_ data: Data, isHeight: Bool) {
        if isHeight {
            guard let reading = DeskProtocol.decodeHeight(data) else { return }
            switch reading {
            case .unknown: currentHeight = nil
            case .centimeters(let height): currentHeight = height
            }
            if isAwaitingResponse, deskState == .idle, currentHeight == Int(selectedHeight.rounded()) {
                finishWaiting()
            }
        } else {
            guard let state = DeskProtocol.decodeStatus(data) else { return }
            deskState = state
            if state != .idle || currentHeight == Int(selectedHeight.rounded()) {
                finishWaiting()
            }
        }
        synchronizeSelection()
    }

    func setPreset(_ preset: DeskPreset) {
        onDragEnded(value: preset.rawValue)
    }

    func sliderEditingChanged(_ isEditing: Bool) {
        isSliderDragging = isEditing
        if !isEditing { onDragEnded(value: selectedHeight) }
    }

    func onDragEnded(value: Double) {
        guard canMove, let data = DeskProtocol.encodeHeight(value) else { return }
        selectedHeight = value.rounded()
        isAwaitingResponse = true
        guard bluetoothManager.sendHeightData(data) else {
            finishWaiting()
            synchronizeSelection()
            return
        }

        guard isAwaitingResponse else { return }
        responseTimeoutTask?.cancel()
        responseTimeoutTask = Task { [weak self] in
            do { try await Task.sleep(for: .seconds(3)) } catch { return }
            guard let self, self.isAwaitingResponse else { return }
            self.errorMessage = "No response from the desk. Check the connection and try again."
            self.finishWaiting()
            self.synchronizeSelection()
        }
    }

    func startScanning() {
        bluetoothManager.startScanning()
    }

    private func finishWaiting() {
        responseTimeoutTask?.cancel()
        responseTimeoutTask = nil
        isAwaitingResponse = false
    }

    private func synchronizeSelection() {
        if !isSliderDragging, !isAwaitingResponse, let currentHeight {
            selectedHeight = Double(currentHeight)
        }
    }
}
