target=$OVNI_TEST_BIN

$target
ovnisort ovni
ovnidump ovni
ovnidump ovni | awk 'NR == 1 {next} NR==2{t=$1} {print $1-t,$2} NR==13{exit}' > found

cat > expected <<EOF
0 OB.
1 OB.
2 OB.
3 OB.
4 OB.
5 OB.
6 OU[
7 OU]
8 OB.
9 OB.
10 OU[
11 OU]
EOF

diff -s found expected

ovniemu ovni
