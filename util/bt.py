from dataclasses import dataclass
import shlex

from typing import Optional

from itertools import islice
from operator import methodcaller

import lldb

# import using: command script import util/bt.p

class LoxDebugException(Exception):
    message: str

    def __init__(self, message: str):
        self.message = message


class Value:

    def __init__(self, sb_value):
        self.value = sb_value

    def __getitem__(self, key):
        if isinstance(key, str):
            result = self.value.GetChildMemberWithName(key)
            if not result.IsValid():
                raise LoxDebugException(f"Unable to get child member for key '{key}'.")
        elif isinstance(key, int):
            result = self.value.GetChildAtIndex(key)
            if not result.IsValid():
                raise LoxDebugException(
                    f"Unable to get child member for index '{key}'."
                )
        else:
            raise TypeError("Key must be either an int or a str")
        return Value(result)

    def read_data(self, method: str):
        data = self.value.GetData()
        if not data.IsValid():
            raise LoxDebugException("Unable to get data for Value.")
        error = lldb.SBError()
        caller = methodcaller(method, error, 0)
        result = caller(data)
        if error.IsValid() and error.Fail():
            raise LoxDebugException(error.GetCString())
        return result

    @property
    def pointer(self) -> int:
        return self.read_data("GetAddress")

    @property
    def uint32(self) -> int:
        return self.read_data("GetUnsignedInt32")

    @property
    def uint64(self) -> int:
        return self.read_data("GetUnsignedInt64")

    @property
    def string(self) -> str:
        summary = self.value.GetSummary()
        if summary and summary.startswith('"') and summary.endswith('"'):
            return summary[1:-1]
        else:
            return summary

    def index(self, idx: int):
        result = self.value.GetValueForExpressionPath(f"[{idx}]")
        if not result.IsValid():
            raise LoxDebugException(f"Unable to get value at index {idx}.")
        return Value(result)

    def dereference(self):
        result = self.value.Dereference()
        if not result.IsValid():
            raise LoxDebugException(f"Unable to dereference value.")
        return Value(result)


@dataclass
class Function:
    start: int
    size: int
    name: str
    value: Value


def check_error(error):
    if error.IsValid() and error.Fail():
        message = error.GetCString()
        message = message if message is not None else "Unknown error"
        raise LoxDebugException(message)


def get_process(debugger):
    target = debugger.GetSelectedTarget()

    if not target:
        raise LoxDebugException("No target is currently selected.")

    process = target.process
    if not process:
        raise LoxDebugException("No process exists for selected target.")

    return process


def print_lox_backtrace(debugger, limit: Optional[int] = None):

    process = get_process(debugger)
    thread = process.GetSelectedThread()
    if not thread.IsValid():
        raise LoxDebugException("No selected thread.")
    frame = thread.frames[0]
    if not frame.IsValid():
        raise LoxDebugException("Unable to get frame from selected thread.")

    code = frame.EvaluateExpression("vm.code")
    if not code.IsValid():
        raise LoxDebugException("Unable to get code index.")
    code = Value(code)

    functionCount = code["functionCount"].uint64
    functionPointer = code["functions"]

    functions: list[Function] = []
    for i in range(functionCount):
        function = functionPointer.index(i).dereference()
        name = function["name"]
        if name.pointer == 0:
            name = "<lox script>"
        else:
            name = name["chars"].string
        functions.append(
            Function(
                function["chunk"]["code"].pointer,
                function["chunk"]["count"].uint32,
                name,
                function,
            )
        )

    def find_function(pc: int):
        return next(
            (
                function
                for function in functions
                if function.start <= pc and pc <= (function.start + function.size * 4)
            ),
            None,
        )

    for frame in islice(thread.frames, limit):
        symbol = frame.GetSymbol()
        if symbol:
            print(frame)
        else:
            function = find_function(frame.GetPC())
            name = "[UNKNOWN]" if function is None else function.name
            if function is not None:
                print(
                    f"frame #{frame.GetFrameID()}: {frame.GetPC():#0{18}x} jit`{name} + {frame.GetPC() - function.start}"
                )
            else:
                print(f"frame #{frame.GetFrameID()}: {frame.GetPC():#0{18}x} [UNKNOWN]")


def print_lox_backtrace_command(debugger, command, result, dictionary):
    """
    Print out the values in an asmlox frame.
    Usage: lox_bt [count]
    """

    args = [int(arg) for arg in shlex.split(command)]

    if len(args) == 0:
        print_lox_backtrace(debugger)
    elif len(args) == 1:
        print_lox_backtrace(debugger, args[0])
    else:
        result.PutCString("Usage: lox_bt [count]")


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        "command script add -f bt.print_lox_backtrace_command lox_bt"
    )
    print("The 'lox_bt' python command has been installed and is ready for use.")
