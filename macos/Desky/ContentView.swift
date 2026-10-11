import SwiftUI
import Combine

enum DeskPreset: Double, CaseIterable {
    case sitting = 75
    case standing = 110

    var imageName: String {
        switch self {
        case .sitting: return "sitting"
        case .standing: return "standing"
        }
    }
}

@MainActor
class ContentViewModel: ObservableObject {
    @Published var currentHeight: Double = 0
    @Published var isSliderDragging: Bool = false
    let range = Double(72)...Double(118)
    init() {
        currentHeight = range.lowerBound
    }

    func setPreset(_ preset: DeskPreset) {
        currentHeight = preset.rawValue
        onDragEnded(value: preset.rawValue)
    }

    func onDragEnded(value: Double) {
        print("Drag ended with final value: \(value)")

    }
}

struct ContentView: View {
    @EnvironmentObject var bluetoothManager: BluetoothManager
    @StateObject private var viewModel = ContentViewModel()
    @Environment(\.openWindow) private var openWindow
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        VStack {
            VStack(spacing: 16) {
                HStack {
                    Text("Desk Height")
                    Spacer()
                    Text("\(Int(viewModel.currentHeight))")
                }
                Slider(
                    value: $viewModel.currentHeight,
                    in: viewModel.range,

                    label: {
                        EmptyView()
                    },
                    minimumValueLabel: {
                        Text("\(Int(viewModel.range.lowerBound))")
                    },
                    maximumValueLabel: {
                        Text("\(Int(viewModel.range.upperBound))")
                    },
                    onEditingChanged: { isEditing in
                        viewModel.isSliderDragging = isEditing
                        if !isEditing {

                            viewModel.onDragEnded(value: viewModel.currentHeight)
                        }
                    }
                )

                VStack {
                    HStack {
                        Text("Presets")
                        Spacer()
                    }

                    HStack {
                        ForEach(DeskPreset.allCases, id: \.self) { preset in
                            Button {
                                viewModel.setPreset(preset)
                            } label: {
                                let isSelected = Int(viewModel.currentHeight) == Int(preset.rawValue)
                                Image(preset.imageName)
                                    .renderingMode(.template)
                                    .resizable()
                                    .aspectRatio(contentMode: .fit)
                                    .frame(width: 36, height: 36)
                                    .foregroundStyle(isSelected ? Color.white : Color.primary)
                                    .padding(8)
                                    .background(
                                        Circle()
                                            .fill(isSelected ? Color.accentColor : Color.clear)
                                    )
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                .onChange(of: viewModel.currentHeight, { _, newValue in
                    let roundedValue = round(newValue)
                    if viewModel.currentHeight != roundedValue {
                        viewModel.currentHeight = roundedValue
                    }
                })
            }

            Divider()

            HStack {
                Image(bluetoothManager.connectedDevice == nil ? "no-bluetooth" : "bluetooth")
                    .renderingMode(.template)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 20, height: 20)

                Spacer()
                Button {
                    dismiss()
                    openWindow(id: "settings")
                } label: {
                    Image(systemName: "gear")
                }
                Button {
                    NSApplication.shared.terminate(nil)
                } label: {
                    Image(systemName: "x.circle")
                }
                .keyboardShortcut("q")
            }
            .buttonStyle(.borderless)

        }
        .padding()
    }
}

#Preview("Light") {
    ContentView()
        .colorScheme(.light)
}

#Preview("Dark") {
    ContentView()
        .colorScheme(.dark)
}
