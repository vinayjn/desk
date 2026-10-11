import Combine
import Foundation

/// Desk events consumed by the UI, independently of the Bluetooth session.
/// Implementations emit publishers and callbacks on the main actor.
@MainActor
protocol DeskTransport: AnyObject {
    var readiness: AnyPublisher<Bool, Never> { get }
    var connectionMessages: AnyPublisher<String, Never> { get }
    var errors: AnyPublisher<String?, Never> { get }
    var dataCallback: (@MainActor (Data, Bool) -> Void)? { get set }

    func startScanning()
    @discardableResult func sendHeightData(_ data: Data) -> Bool
}
