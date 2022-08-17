# CE Disassembler
This is an advanced disassembler for the CE. If you ever wanted to look at the OS without
being able to grab CEmu, this is the right place to be! You can either disassemble the OS
or any assembly/C/ICE program. Including syntax highlighting and fast scrolling, this
program is a must-have for looking inside your calculator.

## Features
- [ ] Disassembling the OS
- [ ] Disassembling any assembly/C/ICE program
- [ ] Syntax highlighting
- [ ] (Fast) scrolling
- [ ] Jumping to address or equate
- [ ] Customizing output format

## Build
To build, download and install the [CE C Toolchain](https://ce-programming.github.io/toolchain/index.html)
and run `make` from the command line. This generates an .8xp file in the `bin/` directory,
which you can move to your calculator or CEmu. To build an appvar with all the OS equates
in it, first run `$ python convert.py` or `$ python3 convert.py` to build the assembly
file, then `fasmg equates.asm` to generate the appvar, `equates.8xv`. Move this file to
your calculator or CEmu. Be sure to put it in RAM! That is necessary, since CEDISASM looks
at the 2 big jump tables in the OS and inserts more equates. After the first disassembly,
you can move the appvar to the Archive, since the equates will stay the same.

## Credits
- [zdis](https://github.com/CE-Programming/zdis/) is used for disassembling code.
- Many people on Cemetech for helping me picking a proper name.

```
20:44:19 <PT_> Hmm I need a nice name for my program
20:46:10 <saxjax_> [D] <LogicalJoe> Dasmce
20:47:37 <Zeroko> Potassium...element that disassembles things...aww, 9 characters. Sodium? Or Fluorine in the opposite direction.
20:48:04 <saxjax_> [D] <LogicalJoe> Potato
20:48:14 <Zeroko> But elements
20:48:35 <Zeroko> I am mostly joking. Not everything on the CE has to be an element.
20:49:45 <PT_> CEDIS?
20:49:54 <Zeroko> I had "CEdis" typed in my input buffer...
20:50:03 <PT_> Heh
20:50:26 <Adriweb> prgmGETMEASM
20:50:37 <PT_> GETMYASM* you mean? :P
20:50:42 <Adriweb> GetMeAsm
20:50:46 <Adriweb> Well sure
20:50:57 <PT_> GIMMEASM
20:51:08 <Adriweb> that works too
20:51:38 <saxjax_> [D] <calc84maniac> why don't you CE dis assembly
20:51:40 <saxjax_> [D] <calc84maniac> *runs*
20:52:42 <Zeroko> deCEasm...pronounce like "decease 'em"
20:53:01 <Zeroko> I like "CEdis" better than that.
20:53:39 <Zeroko> "deCEasm" sounds more like what TI did.
20:57:11 <Adriweb> prgmCEETHRU ?
20:57:17 <Adriweb> Meh
20:59:24 <MateoC> DISASM
21:02:23 <saxjax_> [D] <calc84maniac> CEDISASM, pronounced "see dis asm"
21:03:17 <saxjax_> [D] <kg583> DisCErn
21:03:42 <saxjax_> [D] <LogicalJoe> "Dis-right-now"
21:03:52 <saxjax_> [D] <LogicalJoe> dice*
21:04:23 <saxjax_> [D] <kg583> DICE ain't bad actually
21:07:34 <Zeroko> If it needs CE, I like calc84maniac's suggestion the best so far.
21:16:21 <saxjax_> [D] <epsilon5> Maybe some play on assembly/destruction
21:16:58 <saxjax_> [D] <kg583> DISaster
21:17:03 <saxjax_> [D] <kg583> er, too long
21:17:12 <saxjax_> [D] <LogicalJoe> 8 chars?
21:17:23 <saxjax_> [D] <kg583> I can't count
21:18:05 <saxjax_> [D] <epsilon5> I actually like diCE a lot
21:18:14 <saxjax_> [D] <kg583> it's catchy
21:18:26 <saxjax_> [D] <Oxiti8> Doesnt make much sense for an assembler tho
21:18:33 <saxjax_> [D] <kg583> it's a disassembler
21:18:44 <saxjax_> [D] <epsilon5> Stands for disassembler CE
21:19:01 <saxjax_> [D] <LogicalJoe> maybe diCEr
21:19:02 <saxjax_> [D] <Oxiti8> Yeah but it sounds like a probability simulator program with dice
21:19:20 <saxjax_> [D] <epsilon5> disCE
21:19:25 <saxjax_> [D] <epsilon5> Pronounced "disk"
21:19:39 <saxjax_> [D] <kg583> that's gonna be pronounced "dissee"
21:20:36 <saxjax_> [D] <epsilon5> Fine I'll just keep adding letters
21:20:49 <saxjax_> [D] <epsilon5> disaCE
21:20:55 <saxjax_> [D] <Oxiti8> ASMzxCEh
21:20:56 <saxjax_> [D] <epsilon5> We're not going to say disassCE
21:21:40 <saxjax_> [D] <matkeller19> disaCEm
21:22:26 <saxjax_> [D] <epsilon5> Oh that's good
21:22:47 <saxjax_> [D] <kg583> "dis-ace-em"
21:22:55 <saxjax_> [D] <Oxiti8> Dis a see em
21:24:36 <saxjax_> [D] <epsilon5> Dis a cem
21:24:46 <saxjax_> [D] <epsilon5> Maybe some play on on-calc
21:24:53 <saxjax_> [D] <epsilon5> Like onDisCE
21:25:00 <saxjax_> [D] <epsilon5> Pronounced on 'dis CE
21:25:38 <saxjax_> [D] <epsilon5> On-Calc Disassembler CE
```