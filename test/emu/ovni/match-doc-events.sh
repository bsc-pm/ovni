docs="$OVNI_SOURCE_DIR/doc/user/emulation/events.md"

(
  cat "$docs" | grep -v '^Built on' > old.md
  ovnievents | grep -v '^Built on' > new.md

  diff -up old.md new.md
)
