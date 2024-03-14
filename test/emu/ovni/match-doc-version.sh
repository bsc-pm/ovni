docs="$OVNI_SOURCE_DIR/doc/user/emulation/versions.md"

# Check that the last version of every model appears in the versions.md
# documentation changelog.

(
  # Extract model last versions
  models=$(ovniemu -h 2>&1 | awk '/emulation models/ { p=1; next } p { print $2, $3}')

  if [ -z "$models" ]; then
    echo "ERROR: No models read"
    exit 1
  fi

  echo "$models" | while read -r model version; do
    # Find the first version of the model (the latest)
    first=$(grep -m 1 -- "^- $model " "$docs")
    if [ -z "$first" ]; then
      echo "ERROR: No match of model '$model' in $docs" >&2
      exit 1
    fi

    # And match it to the one reported by the emulator
    matched=$(echo "$first" | grep -- "- $model $version" || true)
    if [ -z "$matched" ]; then
      echo "ERROR: Version $version is not the latest in model $model in $docs" >&2
      exit 1
    fi
  done
)
