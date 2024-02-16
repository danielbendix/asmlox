
.equ VAL_NIL, 0
.equ VAL_BOOL, 1

set_condition:
    cmp w0, VAL_NIL
    b.eq next
    cmp w0, VAL_BOOL
    ccmp w1, #0, #0, eq
next:
    ; after this, eq == false, ne == true

set_condition_bool: ; if we know that we emitted a boolean from a comparison.
    cmp w1, #0
