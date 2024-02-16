
load_constant_up_to_31:
    ldp x0, x1, [x0, #16] ; replace with immediate
    stp x0, x1, [sp, #-16]!

load_constant_after_31:
    add x0, x20, #16 ; replace with immediate
    ldp x0, x1, [x0] ; add max(index - 255, 0)
    stp x0, x1, [sp, #-16]!

