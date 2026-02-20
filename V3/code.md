# TODO

- [ ] Add pointer arithmetic checks
  - Ensure base is pointer/array
  - Compute offset using element size
  - Warn on out-of-bounds constants
- [ ] Basic const propagation
  - Track const vars
  - Fold simple const expressions
  - Flag assigns to const
- [ ] Uninitialized var detection
  - Track init status per symbol
  - Warn on use before assign
- [ ] Function param type matching
  - Check call args vs decl
  - Handle variadics if added
- [ ] Error message upgrades
  - Add source snippets
  - Color output (ANSI)
  - Multiple errors before exit
