#!/usr/bin/env bash
# ============================================================
# X-Phage Golden Test Suite v4.1.0
# Verifies: single-file .xp / Tri-Modular / Mixed combos
# all produce IDENTICAL output. Declaration order independence.
# AeonCoreX Lab
# ============================================================
XPHAGE="${XPHAGE:-/tmp/xphage2}"
DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0; FAIL=0
G="\033[1;32m"; R="\033[1;31m"; Y="\033[1;33m"; C="\033[1;36m"; X="\033[0m"

run() { "$XPHAGE" "$@" 2>&1 | grep -v "^\[ok\]" | grep -v "^\[xp " | grep -v "^\[multi\]"; }

check_eq() {
    local name="$1" got="$2" exp="$3"
    if [ "$got" = "$exp" ]; then
        echo -e "  ${G}✓${X} $name"; PASS=$((PASS+1))
    else
        echo -e "  ${R}✗${X} $name"
        echo -e "    ${Y}expected:${X}"; echo "$exp" | sed 's/^/      /'
        echo -e "    ${Y}got:${X}";      echo "$got" | sed 's/^/      /'
        FAIL=$((FAIL+1))
    fi
}
section() { echo -e "\n${C}── $1 ──${X}"; }

section "Calculator (3 file formats)"
CALC_EXP="calc=42
sq(7)=49
fib(10)=34
max=99"
A=$(run "$DIR/calculator/calc.xp")
B=$(run "$DIR/calculator/calc.xh" "$DIR/calculator/calc.xp0")
C=$(run "$DIR/calculator/calc_split.xp")
check_eq "single .xp"     "$A" "$CALC_EXP"
check_eq ".xh + .xp0"     "$B" "$CALC_EXP"
check_eq "A == B"          "$A" "$B"
check_eq "B == C"          "$B" "$C"

section "Todo Logic (2 formats)"
TODO_EXP="added: Buy milk
added: Write code
done: Buy milk
open=1"
A=$(run "$DIR/todo/todo.xp")
B=$(run "$DIR/todo/todo.xh" "$DIR/todo/todo.xp0")
check_eq "single .xp" "$A" "$TODO_EXP"
check_eq ".xh + .xp0" "$B" "$TODO_EXP"
check_eq "A == B"      "$A" "$B"

section "Event System (2 formats)"
EV_EXP="login: nahid
purchase: book
purchase: pen
total=2"
A=$(run "$DIR/events/events.xp")
B=$(run "$DIR/events/events.xp0")
check_eq "single .xp" "$A" "$EV_EXP"
check_eq ".xp0 only"  "$B" "$EV_EXP"
check_eq "A == B"      "$A" "$B"

section "Config Loader (2 formats)"
CFG_EXP="host=api.xphage.dev
port=443
valid=true"
A=$(run "$DIR/config/config.xp")
B=$(run "$DIR/config/config.xh" "$DIR/config/config.xp0")
check_eq "single .xp" "$A" "$CFG_EXP"
check_eq ".xh + .xp0" "$B" "$CFG_EXP"
check_eq "A == B"      "$A" "$B"

section "Declaration Order Independence (3 orderings)"
ORD_EXP="val=50
label=squared
counter=5
limit=100"
O1=$(run "$DIR/order_independence/order1.xp")
O2=$(run "$DIR/order_independence/order2.xp")
O3=$(run "$DIR/order_independence/order3.xp")
check_eq "order1 (forge→pulse→flux→exec)" "$O1" "$ORD_EXP"
check_eq "order2 (pulse→forge→exec→flux)" "$O2" "$ORD_EXP"
check_eq "order3 (flux→pulse→forge→exec)" "$O3" "$ORD_EXP"
check_eq "1==2" "$O1" "$O2"
check_eq "2==3" "$O2" "$O3"

section "Mixed Combinations (2 input combos)"
MX_EXP="localhost:8080
requests=2"
A=$(run "$DIR/mixed_combo/combo_a.xp")
B=$(run "$DIR/mixed_combo/combo_b.xh" "$DIR/mixed_combo/combo_b.xp0")
check_eq "combo_a: all in .xp" "$A" "$MX_EXP"
check_eq "combo_b: .xh + .xp0" "$B" "$MX_EXP"
check_eq "A == B" "$A" "$B"

section "Realm/Namespace (2 formats — types, nested realm, use)"
REALM_EXP="len_sq=25
area=12.5664
champion"
A=$(run "$DIR/realm_test/realm.xp")
B=$(run "$DIR/realm_test/realm.xh" "$DIR/realm_test/realm.xp0")
check_eq "single .xp"  "$A" "$REALM_EXP"
check_eq ".xh + .xp0"  "$B" "$REALM_EXP"
check_eq "A == B"       "$A" "$B"

echo ""
echo "══════════════════════════════════════════════"
printf "  ${G}PASS${X}: %-3d   ${R}FAIL${X}: %-3d   Total: %d\n" "$PASS" "$FAIL" "$((PASS+FAIL))"
echo "══════════════════════════════════════════════"
if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${G}ALL GOLDEN TESTS PASSED ✓${X}"
    echo -e "  Compiler handles all combinations identically."
    exit 0
else
    echo -e "  ${R}FAILURES: $FAIL ✗${X}"
    exit 1
fi
