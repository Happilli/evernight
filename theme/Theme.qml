pragma Singleton
import QtQuick

QtObject {
    property int mode: 0
    readonly property bool dark: mode === 0 ? Application.styleHints.colorScheme !== Qt.ColorScheme.Light : mode === 2

    readonly property color background: dark ? "#000000" : "#FFFFFF"
    readonly property color onBackgroundColor: dark ? "#FFFFFF" : "#000000"

    readonly property color surface: dark ? "#000000" : "#FFFFFF"
    readonly property color surfaceDim: dark ? "#000000" : "#E8E8E8"
    readonly property color surfaceBright: dark ? "#2E2E2E" : "#FFFFFF"
    readonly property color surfaceContainerLowest: dark ? "#000000" : "#FFFFFF"
    readonly property color surfaceContainerLow: dark ? "#0F0F0F" : "#F7F7F7"
    readonly property color surfaceContainer: dark ? "#1C1C1C" : "#F2F2F2"
    readonly property color surfaceContainerHigh: dark ? "#262626" : "#EBEBEB"
    readonly property color surfaceContainerHighest: dark ? "#2E2E2E" : "#E0E0E0"
    readonly property color surfaceVariant: dark ? "#2E2E2E" : "#E0E0E0"
    readonly property color surfaceTint: primary
    readonly property color onSurfaceColor: dark ? "#FFFFFF" : "#000000"
    readonly property color onSurfaceVariantColor: dark ? "#A3A3A3" : "#5C5C5C"
    readonly property color inverseSurface: dark ? "#E6E6E6" : "#1C1C1C"
    readonly property color inverseOnSurface: dark ? "#1C1C1C" : "#F2F2F2"

    readonly property color outline: dark ? "#6E6E6E" : "#8A8A8A"
    readonly property color outlineVariant: dark ? "#2E2E2E" : "#D6D6D6"

    readonly property color primary: dark ? "#FFFFFF" : "#3A3A3A"
    readonly property color onPrimaryColor: dark ? "#000000" : "#FFFFFF"
    readonly property color primaryContainer: dark ? "#3D3D3D" : "#CFCFCF"
    readonly property color onPrimaryContainerColor: dark ? "#FFFFFF" : "#000000"
    readonly property color primaryFixed: "#E6E6E6"
    readonly property color primaryFixedDim: "#B8B8B8"
    readonly property color onPrimaryFixedColor: "#000000"
    readonly property color onPrimaryFixedVariantColor: "#3D3D3D"
    readonly property color inversePrimary: dark ? "#3A3A3A" : "#FFFFFF"

    readonly property color secondary: dark ? "#A3A3A3" : "#5C5C5C"
    readonly property color onSecondaryColor: dark ? "#000000" : "#FFFFFF"
    readonly property color secondaryContainer: dark ? "#2E2E2E" : "#E4E4E4"
    readonly property color onSecondaryContainerColor: dark ? "#FFFFFF" : "#000000"
    readonly property color secondaryFixed: "#E4E4E4"
    readonly property color secondaryFixedDim: "#B0B0B0"
    readonly property color onSecondaryFixedColor: "#000000"
    readonly property color onSecondaryFixedVariantColor: "#3D3D3D"

    readonly property color tertiary: dark ? "#8A8A8A" : "#6E6E6E"
    readonly property color onTertiaryColor: dark ? "#000000" : "#FFFFFF"
    readonly property color tertiaryContainer: dark ? "#3D3D3D" : "#D6D6D6"
    readonly property color onTertiaryContainerColor: dark ? "#FFFFFF" : "#000000"
    readonly property color tertiaryFixed: "#DCDCDC"
    readonly property color tertiaryFixedDim: "#A8A8A8"
    readonly property color onTertiaryFixedColor: "#000000"
    readonly property color onTertiaryFixedVariantColor: "#3D3D3D"

    readonly property color error: dark ? "#D0D0D0" : "#3A3A3A"
    readonly property color onErrorColor: dark ? "#000000" : "#FFFFFF"
    readonly property color errorContainer: dark ? "#3D3D3D" : "#CFCFCF"
    readonly property color onErrorContainerColor: dark ? "#FFFFFF" : "#000000"

    readonly property color scrim: "#000000"
    readonly property color shadow: "#000000"
}
