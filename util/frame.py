from enum import IntEnum
import struct
import shlex

from typing import Optional

import lldb


class LoxValueType(IntEnum):
    VAL_NIL = 0
    VAL_BOOL = 1
    VAL_NUMBER = 2
    VAL_OBJ = 3


class LoxDebugException(Exception):
    message: str

    def __init__(self, message: str):
        self.message = message


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


def print_lox_frame(
    debugger, frame_index: Optional[int] = None, thread_index: Optional[int] = None
):
    process = get_process(debugger)

    def read_unsigned(address, count: int):
        nonlocal process
        error = lldb.SBError()
        result = process.ReadUnsignedFromMemory(address, count, error)
        check_error(error)
        return result

    def read_pointer(address):
        nonlocal process
        error = lldb.SBError()
        result = process.ReadPointerFromMemory(address, error)
        check_error(error)
        return result

    def read_data(address, count: int):
        nonlocal process
        error = lldb.SBError()
        result = process.ReadMemory(address, count, error)
        check_error(error)
        return result

    if thread_index is not None:
        if thread_index >= process.GetNumThreads():
            raise LoxDebugException("Thread index is out of bounds.")
        thread = process.threads[thread_index]
    else:
        thread = process.GetSelectedThread()

    if frame_index is not None:
        if frame_index >= thread.GetNumFrames():
            raise LoxDebugException("Frame index is out of bounds.")
        frame = thread.frames[frame_index]
    else:
        frame = thread.GetSelectedFrame()
        frame_index = frame.GetFrameID()

    fp = frame.fp
    first = fp - 32
    if frame_index == 0:
        # We're inspecting the top frame, so read values from frame pointer to stack pointer.
        sp = frame.sp
        last = sp
    else:
        frame_below = thread.frames[frame_index - 1]

        if frame_index == 1 and fp == frame_below.fp:
            # Bottom frame has not spilled a frame and link pointer.
            # asmlox functions can only call functions that will store values
            # before spilling a frame and link pointer.
            sp = frame.sp
            last = sp
        else:
            last = frame_below.fp + 16

    pointer = first
    while pointer >= last:
        try:
            value_type = LoxValueType(read_unsigned(pointer, 4))

            debug_string_header = f"{pointer:#0{18}x}: "

            value_description = ""
            if value_type == LoxValueType.VAL_NIL:
                value_description = "nil"
            elif value_type == LoxValueType.VAL_BOOL:
                value_description = "true" if read_unsigned(pointer + 8, 1) else "false"
            elif value_type == LoxValueType.VAL_NUMBER:
                value_description = str(
                    struct.unpack("@d", read_data(pointer + 8, 8))[0]
                )
            elif value_type == LoxValueType.VAL_OBJ:
                object_pointer = read_pointer(pointer + 8)
                value_description = "OBJECT " + hex(object_pointer)

            print(debug_string_header + value_description)

        except LoxDebugException:
            pass

        pointer -= 16


def print_lox_frame_command(debugger, command, result, dictionary):
    """
    Print out the values in an asmlox frame.
    Usage: lox_frame [frame] [thread]
    """

    args = [int(arg) for arg in shlex.split(command)]

    if len(args) == 0:
        print_lox_frame(debugger)
    elif len(args) == 1:
        print_lox_frame(debugger, args[0])
    elif len(args) == 2:
        print_lox_frame(debugger, args[0], args[1])
    else:
        result.PutCString("Usage: lox_frame [frame] [thread]")


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        "command script add -f frame.print_lox_frame_command lox_frame"
    )
    print("The 'lox_frame' python command has been installed and is ready for use.")
