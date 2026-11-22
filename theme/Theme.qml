pragma Singleton
import QtQuick
import Evernight

QtObject {
    property int mode: {
        const m = Number(Store.get("themeMode", 0));
        return m >= 0 && m <= 2 ? m : 0;
    }
    readonly property bool dark: mode === 0 ? Application.styleHints.colorScheme !== Qt.ColorScheme.Light : mode === 2
    readonly property var t: dark ? ThemeLoader.dark : ThemeLoader.light

    readonly property color background: t.background
    readonly property color onBackgroundColor: t.onBackgroundColor
    readonly property color surface: t.surface
    readonly property color surfaceDim: t.surfaceDim
    readonly property color surfaceBright: t.surfaceBright
    readonly property color surfaceContainerLowest: t.surfaceContainerLowest
    readonly property color surfaceContainerLow: t.surfaceContainerLow
    readonly property color surfaceContainer: t.surfaceContainer
    readonly property color surfaceContainerHigh: t.surfaceContainerHigh
    readonly property color surfaceContainerHighest: t.surfaceContainerHighest
    readonly property color surfaceVariant: t.surfaceVariant
    readonly property color surfaceTint: primary
    readonly property color onSurfaceColor: t.onSurfaceColor
    readonly property color onSurfaceVariantColor: t.onSurfaceVariantColor
    readonly property color inverseSurface: t.inverseSurface
    readonly property color inverseOnSurface: t.inverseOnSurface
    readonly property color outline: t.outline
    readonly property color outlineVariant: t.outlineVariant
    readonly property color primary: t.primary
    readonly property color onPrimaryColor: t.onPrimaryColor
    readonly property color primaryContainer: t.primaryContainer
    readonly property color onPrimaryContainerColor: t.onPrimaryContainerColor
    readonly property color primaryFixed: t.primaryFixed
    readonly property color primaryFixedDim: t.primaryFixedDim
    readonly property color onPrimaryFixedColor: t.onPrimaryFixedColor
    readonly property color onPrimaryFixedVariantColor: t.onPrimaryFixedVariantColor
    readonly property color inversePrimary: t.inversePrimary
    readonly property color secondary: t.secondary
    readonly property color onSecondaryColor: t.onSecondaryColor
    readonly property color secondaryContainer: t.secondaryContainer
    readonly property color onSecondaryContainerColor: t.onSecondaryContainerColor
    readonly property color secondaryFixed: t.secondaryFixed
    readonly property color secondaryFixedDim: t.secondaryFixedDim
    readonly property color onSecondaryFixedColor: t.onSecondaryFixedColor
    readonly property color onSecondaryFixedVariantColor: t.onSecondaryFixedVariantColor
    readonly property color tertiary: t.tertiary
    readonly property color onTertiaryColor: t.onTertiaryColor
    readonly property color tertiaryContainer: t.tertiaryContainer
    readonly property color onTertiaryContainerColor: t.onTertiaryContainerColor
    readonly property color tertiaryFixed: t.tertiaryFixed
    readonly property color tertiaryFixedDim: t.tertiaryFixedDim
    readonly property color onTertiaryFixedColor: t.onTertiaryFixedColor
    readonly property color onTertiaryFixedVariantColor: t.onTertiaryFixedVariantColor
    readonly property color error: t.error
    readonly property color onErrorColor: t.onErrorColor
    readonly property color errorContainer: t.errorContainer
    readonly property color onErrorContainerColor: t.onErrorContainerColor
    readonly property color scrim: t.scrim
    readonly property color shadow: t.shadow
}
