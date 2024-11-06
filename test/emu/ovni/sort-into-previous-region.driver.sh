target=$OVNI_TEST_BIN

$target
ovnisort ovni
ovnidump ovni
ovnidump ovni | awk '{print $1,$2}' > found

cat > expected <<EOF
1 OHx
100 OB.
101 OB.
102 OB.
103 OB.
104 OB.
105 OB.
106 OU[
107 OU]
108 OB.
109 OB.
110 OU[
111 OU]
200 OHe
EOF

diff -s found expected

ovniemu ovni
