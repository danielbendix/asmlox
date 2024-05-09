#define BINARY_OP(v1, v2, valueType, op) \
    if ((v1.type != v2.type) + (v1.type != VAL_NUMBER)) { \
        runtimeError("Operands must be numbers."); \
    } \
    double b = AS_NUMBER(v2); \
    double a = AS_NUMBER(v1); \
    return valueType(a op b); \
