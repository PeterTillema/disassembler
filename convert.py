import re

equates = []

with open('ti84pce.inc', 'r') as f:
    re_right = re.compile(r'(\w+?)\s+equ\s+0([0-9A-F]{6})h')

    for line in f:
        obj = re.match(re_right, line)
        if obj:
            equates.append((int(obj.group(2), 16), obj.group(1)))

equates = sorted(equates, key=lambda tup: tup[0])
print(len(equates))

output = """include 'includes/commands.alm'
include 'includes/ez80.alm'
include 'includes/tiformat.inc'

format ti appvar 'DISASM'

\tdl {}
""".format(len(equates))

for equate in equates:
    output += '\tdl 0x{:06X}, {}\n'.format(equate[0], equate[1])

output += '\n\n'

for equate in equates:
    output += equate[1] + ':\n\tdb "' + equate[1] + '", 0\n'

with open('equates.asm', 'w') as f:
    f.write(output)
