# QtMaps Design Assets

`design/` is source material for the QtMaps visual system; it is not a runtime
resource directory until an asset is explicitly added to `qml/resources.qrc`.

## Icons

- Store one icon per lowercase kebab-case SVG in `icons/`.
- Use `viewBox="0 0 24 24"`, a 2 px rounded stroke, and `currentColor` for
  single-colour geometry. Do not embed fonts, raster data, or external URLs.
- Runtime size must come from Theme pixel/spacing tokens. Colour comes from
  semantic Theme roles (`accent`, `textPrimary`, `textSecondary`, `warning`,
  `error`, `success`), never a duplicated literal.
- Add an icon only with a visible text label or accessible action name where
  its meaning is not universally obvious. Keep map-render icons out of this
  directory's runtime integration scope.

## Adding an icon

1. Draw on the 24 px grid and preserve at least a 2 px visual inset.
2. Add the SVG and verify it contains no embedded font, bitmap, or URL.
3. Document its semantic role and add focused QML geometry/keyboard coverage
   before adding it to the runtime resource collection.
4. Keep design-source, resource-list, and QML-component changes in small,
   independently tested commits.

## Other assets

`app-icon/` contains application-mark source and exports. `brand/` records the
palette and typography already defined by `qml/theme/Theme.qml`; it does not
introduce a new font or a second theme contract.
