#!/usr/bin/env bash
# Read from /dev/vdaq0 with several readers at once and check that no sample is duplicated.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULE="$SCRIPT_DIR/../driver/virtual_daq.ko"
READERS=8
SAMPLES_EACH=10000          # 8 x 10000 = 80000, less than the 100000 period
OUT="$(mktemp -d)"

sudo rmmod virtual_daq 2>/dev/null
sudo insmod "$MODULE" || { echo "FAIL: insmod"; exit 1; }

for i in $(seq 1 "$READERS"); do
    sudo od -An -v -t d4 -N $((SAMPLES_EACH * 4)) /dev/vdaq0 > "$OUT/reader$i" &
done
wait

cat "$OUT"/reader* | tr -s ' ' '\n' | grep -v '^$' > "$OUT/all"
total=$(wc -l < "$OUT/all")
dups=$(sort "$OUT/all" | uniq -d | wc -l)

echo "samples read: $total (expected $((READERS * SAMPLES_EACH)))"
echo "duplicate values: $dups"

sudo rmmod virtual_daq
rm -rf "$OUT"

if [ "$total" -eq $((READERS * SAMPLES_EACH)) ] && [ "$dups" -eq 0 ]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    exit 1
fi