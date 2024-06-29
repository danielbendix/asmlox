from enum import IntEnum
from typing import Type
import struct

import lldb


class LoxValueType(IntEnum):
    VAL_NIL = 0
    VAL_BOOL = 1
    VAL_NUMBER = 2
    VAL_OBJ = 3


def check_error(error: lldb.SBError, error_type: Type[Exception]):
    if error.IsValid() and error.Fail():
        message = error.GetCString()
        message = message if message is not None else "Unknown error"
        raise error_type(message)


def read_unsigned(process: lldb.SBProcess, address: int, count: int):
    error = lldb.SBError()
    result = process.ReadUnsignedFromMemory(address, count, error)
    check_error(error, ValueError)
    return result


def read_pointer(process: lldb.SBProcess, address: int):
    error = lldb.SBError()
    result = process.ReadPointerFromMemory(address, error)
    check_error(error, ValueError)
    return result


def read_data(process: lldb.SBProcess, address: int, count: int):
    error = lldb.SBError()
    result = process.ReadMemory(address, count, error)
    check_error(error, ValueError)
    return result


def describe_lox_value(process: lldb.SBProcess, pointer: int) -> str:
    value_type = LoxValueType(read_unsigned(process, pointer, 4))

    value_description = ""
    if value_type == LoxValueType.VAL_NIL:
        value_description = "nil"
    elif value_type == LoxValueType.VAL_BOOL:
        value_description = (
            "true" if read_unsigned(process, pointer + 8, 1) else "false"
        )
    elif value_type == LoxValueType.VAL_NUMBER:
        value_description = str(
            struct.unpack("@d", read_data(process, pointer + 8, 8))[0]
        )
    elif value_type == LoxValueType.VAL_OBJ:
        object_pointer = read_pointer(process, pointer + 8)
        value_description = "OBJECT " + hex(object_pointer)

    return value_description
