#!/usr/bin/env python3

import argparse
import csv
import sys

if sys.hexversion < 0x03000000:
    raise Exception('Cannot run on anything earlier than Python 3.0')


def get_messages(handle):
    read_lines = []

    for row in csv.reader(handle, delimiter='\t', quotechar='\\',
                          dialect='unix'):
        if len(row) == 0:
            continue
        stripped = row[0].strip()
        if stripped.startswith('#'):
            items = stripped.split()
            if items[0] == '#include':
                with open(' '.join(items[1:]), 'r') as include_handle:
                    include_lines = get_messages(include_handle)
                    if len(include_lines) > 0:
                        read_lines += include_lines
            continue
        if len(row) != 3:
            continue
        if row[0].strip().startswith('//'):
            continue
        if row[1] not in ('0', '1'):
            continue
        row[2] = row[2].strip()
        if not (row[2].startswith('"') and row[2].endswith('"')):
            continue
        row[2] = row[2][1:-1].replace('\\r', '\r').replace('\\n', '\n').replace(
            '\\t', '\t').replace('\\"', '"').replace('\\\\', '\\')

        read_lines.append(row)

    return read_lines


parser = argparse.ArgumentParser(
    description='Pack Bus Pirate strings into something that can be '
                'included by the firmware.')
parser.add_argument('source', metavar='TEXT',
                    type=argparse.FileType('r'), help='the string file')
parser.add_argument('outbase', metavar='OUTPUT', type=str,
                    help='the base filename for output files')
parser.add_argument('guard', metavar='GUARD', type=str,
                    help='an additional marker to put in the C header '
                         '#include guard block')

args = parser.parse_args()
lines = get_messages(args.source)

with open(args.outbase + '.s', 'w') as assembly_output:
    for row in sorted(lines):
        assembly_output.write('\t; %s\n' % row[0])
        assembly_output.write('\t.section .text.%s, code\n' % row[0])
        assembly_output.write('\t.global _%s_str\n' % row[0])
        assembly_output.write('_%s_str:\n' % row[0])
        data = row[2].replace('\\', '\\\\').replace('\n', '\\n').replace(
            '\r', '\\r').replace('"', '\\"').replace('\t', '\\t')
        assembly_output.write('\t.pascii "%s"\n\n' % data)

offset = 0
BUFFER_WRITE_CALL = 'bp_message_write_buffer'
LINE_WRITE_CALL = 'bp_message_write_line'

with open(args.outbase + '.h', 'w') as header_output:
    header_output.write('#ifndef BP_MESSAGES_%s_H\n' % args.guard.upper())
    header_output.write('#define BP_MESSAGES_%s_H\n\n' % args.guard.upper())

    for row in sorted(lines):
        call = BUFFER_WRITE_CALL if row[1] == '0' else LINE_WRITE_CALL
        header_output.write('void %s_str(void);\n' % row[0])
        header_output.write('#define %s %s(__builtin_tbladdress(%s_str), %d)\n' %
                            (row[0], call, row[0], len(row[2])))
        offset += len(row[2])

    header_output.write('\n#endif /* BP_MESSAGES_%s_H */\n' %
                        args.guard.upper())

# vim:sts=2:sw=2:ts=2:et:syn=python:fdm=marker:ff=unix:fenc=utf-8:number:cc=80
