# Load virtual_daq with each forced init failure and check that nothing is left behind.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULE="$SCRIPT_DIR/../driver/virtual_daq.ko"
failures=0

# Start from a clean state
sudo rmmod virtual_daq 2>/dev/null

for step in 1 2 3 4; do
    if sudo insmod "$MODULE" fail_step="$step" 2>/dev/null; then
        echo "FAIL step $step: insmod succeeded but should have failed"
        sudo rmmod virtual_daq
        failures=$((failures + 1))
        continue
    fi

    leftovers=""
    lsmod | grep -qw virtual_daq        && leftovers+=" module"
    grep -qw virtual_daq /proc/devices  && leftovers+=" device-number"
    [ -e /sys/class/virtual_daq ]       && leftovers+=" class"
    [ -e /dev/vdaq0 ]                   && leftovers+=" /dev/vdaq0"

    if [ -n "$leftovers" ]; then
        echo "FAIL step $step: left behind:$leftovers"
        failures=$((failures + 1))
    else
        echo "PASS step $step: clean failure"
    fi
done

# Finally, a normal load should still work
if sudo insmod "$MODULE" && [ -e /dev/vdaq0 ]; then
    echo "PASS normal load: /dev/vdaq0 created"
    sudo rmmod virtual_daq
else
    echo "FAIL normal load"
    failures=$((failures + 1))
fi

echo "$failures failure(s)"
exit "$failures"