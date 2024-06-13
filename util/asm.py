import argparse
import re
import os
from typing import Tuple

# FIXME: Support string literals with commas inside them."

def is_ascii(string: str) -> bool:
    return all(ord(c) < 128 for c in string)

def check_string(string: str):
    if not is_ascii(string):
        raise Exception(f"String '{string}' contains non-ASCII characters.")

class Platform:
    def function(self, *args: str) -> list[str]:
        raise Exception("Not implemented")

    def end_function(self, *args: str) -> list[str]:
        raise Exception("Not implemented")

    def string(self, *args: str) -> list[str]:
        raise Exception("Not implemented")

    def load_string(self, *args: str) -> list[str]:
        raise Exception("Not implemented")

    def footer(self, *args: str) -> list[str]:
        raise Exception("Not implemented")

class MacOS(Platform):
    def __init__(self):
        self.string_label_idx = 0

    def get_string_idxs(self) -> Tuple[int, int]:
        result = (self.string_label_idx, self.string_label_idx + 1)
        self.string_label_idx += 2
        return result

    def section(self, *args: str) -> list[str]:
        section_type = args[0]
        match section_type:
            case 'CODE':
                section = "    .section __TEXT,__text,regular,pure_instructions\n"
            case 'STRINGS':
                section = "    .section	__TEXT,__cstring,cstring_literals\n"
            case _:
                raise Exception(f"Unknown section type '{section_type}'.")
        return [section]

    def function(self, *args: str) -> list[str]:
        name = '_' + args[0]
        return [
            f"    .globl {name}\n",
            "    .p2align 2\n",
            f"{name}:\n"
        ]

    def end_function(self, *args: str) -> list[str]:
        # No function end required on macOS
        return []

    def string(self, *args: str) -> list[str]:
        label = args[0]
        string = args[1]
        if string and string[0] == '"' and string[-1] == '"':
            string = string[1:-1]
        check_string(string)
        return [
            f"{label}:\n",
            f"    .asciz \"{string}\"\n",
        ]

    def load_string(self, *args: str) -> list[str]:
        register = args[0]
        label = args[1]
        (label_idx_0, label_idx_1) = self.get_string_idxs()
        label_0 = f"Lloh{label_idx_0}"
        label_1 = f"Lloh{label_idx_1}"
        return [
            f"{label_0}:\n",
            f"    adrp    {register}, {label}@PAGE\n",
            f"{label_1}:\n",
            f"    add {register}, {register}, {label}@PAGEOFF\n",
            f"    .loh AdrpAdd    {label_0}, {label_1}\n",
        ]

    def footer(self, *args: str):
        return [
            # Let linker know that symbols can be split up.
            ".subsections_via_symbols\n",
        ]

class Linux(Platform):
    def section(self, *args: str) -> list[str]:
        section_type = args[0]
        match section_type:
            case 'CODE':
                section = "    .text\n"
            case 'STRINGS':
                section = "    .section	.rodata.str1.1,\"aMs\",@progbits,1\n"
            case _:
                raise Exception(f"Unknown section type '{section_type}'.")
        return [section]

    def function(self, *args: str) -> list[str]:
        name = args[0]
        return [
            f"    .globl  {name}\n",
            "    .p2align 2\n",
            f"    .type   {name},@function\n",
            f"{name}:\n"
        ]

    def end_function(self, *args: str) -> list[str]:
        name = args[0]
        label = f"Lfunc_{name}_end"
        return [
            f"{label}:\n",
            f"    .size op_add, {label} - {name}\n",
        ]

    def string(self, *args: str) -> list[str]:
        label = args[0]
        string = args[1]
        if string and string[0] == '"' and string[-1] == '"':
            string = string[1:-1]
        check_string(string)
        return [
            f"    .type {label},@object\n"
            f"{label}:\n",
            f"    .asciz \"{string}\"\n",
            f"    .size {label}, {len(string) + 1}\n",
        ]

    def load_string(self, *args: str) -> list[str]:
        register = args[0]
        label = args[1]
        return [
            f"    adrp    {register}, {label}\n",
            f"    add {register}, {register}, :lo12:{label}\n",
        ]

    def footer(self, *args: str) -> list[str]:
        return [
            # Explicitly disable executable stack.
            ".section .note.GNU-stack,\"\",@progbits\n",
            # Build address significance table.
            ".addrsig\n",
        ]

def process_directive(line, platform):
    """
    Process a single directive line and return the corresponding output lines.
    
    :param line: The line containing the directive.
    :param platform: An instance of Platform.
    :return: A list of lines to replace the directive.
    """
    directive_match = re.match(r'\$(\w+)\((.*)\)', line.strip())
    if not directive_match:
        return [line]
    
    directive_type, directive_params = directive_match.groups()
    params = [param.strip() for param in directive_params.split(',')]

    match directive_type:
        case 'SECTION':
            return platform.section(*params)
        case 'FUNCTION':
            return platform.function(*params)
        case 'END_FUNCTION':
            return platform.end_function(*params)
        case 'STRING':
            return platform.string(*params)
        case 'LOAD_STRING':
            return platform.load_string(*params)
        case 'FOOTER':
            return platform.footer(*params)
    
    raise Exception(f"Invalid directive {directive_type}")

def process_lines(input_lines, platform):
    """
    Process input lines and replace directives according to settings.
    
    :param input_lines: A list of input lines to process.
    :param platform: An instance of Platform.
    :return: A list of processed output lines.
    """
    output_lines = []
    for line in input_lines:
        stripped_line = line.lstrip()
        if stripped_line.startswith('$'):
            output_lines.extend(process_directive(line, platform))
        else:
            output_lines.append(line)
    return output_lines

def main(input_file_path, output_file_path, platform):
    """
    Main function to read the input file, process it, and write the output file.
    
    :param input_file_path: Path to the input file.
    :param output_file_path: Path to the output file.
    :param platform: An instance of Platform.
    """
    with open(input_file_path, 'r') as input_file:
        input_lines = input_file.readlines()
    
    output_lines = process_lines(input_lines, platform)
    
    with open(output_file_path, 'w') as output_file:
        output_file.writelines(output_lines)

def detect_platform() -> Platform:
    import platform

    uname = platform.uname()

    if uname.machine != 'arm64':
        raise Exception("Machine is not arm64.")

    match uname.system:
        case 'Darwin':
            return MacOS()
        case 'Linux':
            return Linux()

    raise Exception(f"Unsupported system '{uname.system}'")

def get_platform(name: str) -> Platform:
    match name.lower():
        case 'macos':
            return MacOS()
        case 'linux':
            return Linux()

    raise Exception(f"Unsupported platform '{name}'")

def replace_extension(file_path, new_extension):
    """
    Replace the file extension of the given file path with a new extension.
    
    :param file_path: The original file path.
    :param new_extension: The new file extension (including the dot, e.g., '.txt').
    :return: The file path with the new extension.
    """
    base = os.path.splitext(file_path)[0]
    return base + new_extension

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Prepares platform-independent assembly for a given platform.")

    parser.add_argument("file", help="Path of the input file.")
    parser.add_argument("-o", "--output", help="Path of the output file.")
    parser.add_argument("-p", "--platform", help="The platform to output assembly for. Currently supported are [macos, linux].")

    return parser.parse_args()

# Example usage
if __name__ == "__main__":
    args = parse_args()

    if args.platform is not None:
        platform = get_platform(args.platform)
    else:
        platform = detect_platform()

    file = args.file

    if args.output is not None:
        output = args.output
    else:
        output = replace_extension(file, '.s')

    main(file, output, platform)
