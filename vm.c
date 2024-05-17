#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

#include "ops.h"

VM vm;

void initVM()
{
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    
    initTable(&vm.globals);
    initTable(&vm.strings);

    vm.initString = NULL;
    vm.initString = copyString("init", 4);

    vm.code = NULL;
    vm.nextCode = NULL;
    vm.codeEnd = NULL;
}

void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
}

void setMainReturnAddress(void *returnAddress)
{
    vm.mainReturnAddress = returnAddress;
}

InterpretResult start_script(ObjClosure *closure);

InterpretResult interpret(const char *source, bool isREPL)
{
    ObjFunction *function = compile(source, isREPL);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    pushGCRoot(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    popGCRoot();
    pushGCRoot(OBJ_VAL(closure));

    return start_script(closure);
}
