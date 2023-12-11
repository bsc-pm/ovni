# Test ovniver as-is
(
  ovniver
)

# Test LD_LIBRARY_PATH check in ovniver
(
  export LD_LIBRARY_PATH=/hopefully/nothing/is/here
  ovniver | grep "LD_LIBRARY_PATH set to"
  unset LD_LIBRARY_PATH
  ovniver | grep "LD_LIBRARY_PATH not set"
)
