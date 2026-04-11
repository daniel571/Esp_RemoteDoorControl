✅ Use these first (safe and recommended)
1. GPIO4

Best general-purpose pin

No boot conflicts
✔ Use without worry.

2. GPIO16

Works as GPIO input if you add external pull-up (10k)

No interrupts on edges (important!)
✔ Good for stable digital input.

⚠️ Usable, but with boot restrictions
3. GPIO0

MUST be HIGH at boot

If low on boot → ESP enters flashing mode

As input:

Requires 10k pull-up

External circuit must not drive low during reset
⚠️ Use only if your sensor/input is guaranteed not to pull low during reset.

4. GPIO2

MUST be HIGH at boot

If low on boot → ESP does not boot correctly

As input:

Requires built-in pull-up or external 10k
⚠️ Same rule: input must not pull low at reset.