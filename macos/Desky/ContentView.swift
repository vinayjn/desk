import SwiftUI
import Combine

enum Automation: String, CaseIterable {
    case calendar
    case clock = "60.arrow.trianglehead.clockwise"
}

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

struct ContentView: View {
    @ObservedObject var viewModel: ContentViewModel
    @Environment(\.openWindow) private var openWindow
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        VStack {
            VStack(spacing: 16) {
                HStack {
                    Text("Desk Height")
                    Spacer()
                    Text(viewModel.isSliderDragging
                         ? "\(Int(viewModel.selectedHeight.rounded())) cm"
                         : viewModel.currentHeight.map { "\($0) cm" } ?? "—")
                }
                .fontWeight(.medium)
                Slider(
                    value: $viewModel.selectedHeight,
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
                        viewModel.sliderEditingChanged(isEditing)
                    }
                )
                .accessibilityLabel("Desk height")
                .accessibilityValue("\(Int(viewModel.selectedHeight.rounded())) centimeters")

                VStack {
                    HStack {
                        Text("Presets")
                            .fontWeight(.medium)
                        Spacer()
                    }

                    HStack {
                        ForEach(DeskPreset.allCases, id: \.self) { preset in
                            Button {
                                viewModel.setPreset(preset)
                            } label: {
                                let isSelected = viewModel.currentHeight == Int(preset.rawValue)
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
                            .accessibilityLabel(preset == .sitting ? "Sitting height" : "Standing height")
                        }
                    }
                }
            }
            .disabled(!viewModel.canMove)

            if let error = viewModel.displayedErrorMessage {
                Text(error)
                    .font(.caption)
                    .foregroundStyle(.red)
                    .fixedSize(horizontal: false, vertical: true)
            }

            Divider()
            VStack {
                HStack {
                    Text("Schedule")
                        .fontWeight(.medium)
                    Spacer()
                }

                HStack(alignment: .center) {
                    ForEach(Automation.allCases, id: \.self) { automation in
                        Button {
                            viewModel.selectedAutomation = automation
                        } label: {
                            let isSelected = viewModel.selectedAutomation == automation
                            Image(systemName: automation.rawValue)
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

            Divider()

            HStack(alignment: .center) {
                Image(viewModel.isConnected ? "bluetooth" : "no-bluetooth")
                    .renderingMode(.template)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 18, height: 18)

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
        .frame(width: 280)
        .padding()
        .onAppear {
            viewModel.startScanning()
        }
    }
}
