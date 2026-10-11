import Foundation

enum Constants {
    static let DESK_SERVICE_ID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
    static let DESK_SET_HEIGHT_CHAR_ID = "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
    static let DESK_GET_HEIGHT_CHAR_ID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
    static let DESK_GET_STATUS_CHAR_ID = "9a8ca5fa-7d85-4e8e-9e3a-6c8d3f2b1e4d"
}

enum DeskStatus: String {
    case idle = "IDLE"
    case moving = "MOVING"
    case error = "ERROR"
}

enum DeskProtocol {
    enum HeightReading: Equatable {
        case unknown
        case centimeters(Int)
    }

    static let heightRange = 72...118

    static func encodeHeight(_ height: Double) -> Data? {
        guard height.isFinite, (Double(heightRange.lowerBound)...Double(heightRange.upperBound)).contains(height) else {
            return nil
        }
        return Data([UInt8(height.rounded())])
    }

    static func decodeHeight(_ data: Data) -> HeightReading? {
        guard data.count == 1, let byte = data.first else { return nil }
        if byte == 0 { return .unknown }
        let height = Int(byte)
        return heightRange.contains(height) ? .centimeters(height) : nil
    }

    static func decodeStatus(_ data: Data) -> DeskStatus? {
        guard let value = String(data: data, encoding: .utf8) else { return nil }
        return DeskStatus(rawValue: value.uppercased())
    }
}
