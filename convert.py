import re

equates = []


def is_jump_table(eq):
    return 0x000084 <= eq[0] <= 0x00063C or 0x020104 <= eq[0] <= 0x022278


with open('ti84pce.inc', 'r') as f:
    re_right = re.compile(r'(\w+?)\s+equ\s+0([0-9A-F]{6})h')

    for line in f:
        obj = re.match(re_right, line)
        if obj:
            equates.append((int(obj.group(2), 16), obj.group(1)))

equates = sorted(equates, key=lambda tup: tup[0])
print('Amount of equates:', len(equates))

jump_table_equates = len([x for x in equates if is_jump_table(x)])
print('Amount of jump table entries:', jump_table_equates)

output = """include 'includes/commands.alm'
include 'includes/ez80.alm'
include 'includes/tiformat.inc'

format ti appvar 'DISASM'

\tdl {}, {}
""".format(len(equates), jump_table_equates)

for equate in equates:
    output += '\tdl 0x{:06X}, {}'.format(equate[0], equate[1])

    if is_jump_table(equate):
        output += ' + 1'
    output += '\n'

output += '\n\trb {}'.format(jump_table_equates * 6)

output += '\n\n'

for equate in equates:
    output += equate[1] + ':\n\tdb "'

    if is_jump_table(equate):
        output += '_'

    output += equate[1] + '", 0\n'

with open('equates.asm', 'w') as f:
    f.write(output)
