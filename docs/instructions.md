# Slang instruction set 
Heavily inspired by Lua's instruction set :)

## Instruction Shapes
```
    6         8          9           9
+--------+---------+-----------+-----------+
| OpCode |    A    |     B     |     C     |
+--------+---------+-----------+-----------+
| OpCode |    A    |           Bx          |
+--------+---------+-----------+-----------+
| OpCode |    A    |          sBx          |
+--------+---------+-----------------------+

```
- OpABC: OpCode + A + B + C
- OpABx:  OpCode + A + Bx
- OpAsBx: OpCode + A + sBx

## Instruction Reference
```
Return          return RC[B]
Self            R[A] <- self
Call            R[C] <- RO[B](R[C], ..., R[C + A - 1])
Jmp             PC <- PC + sBx 
JmpIfFalse      PC <- !R[A] ? PC + sBx : PC
JmpIfTrue       PC <-  R[A] ? PC + sBx : PC
Constant        R[A] <- C[Bx]
Object          R[A] <- O[Bx]
Closure         R[A] <- closure O[Bx]
GetUpval        R[A] <- Upval[Bx]
SetUpval        Upval[A] <- RC[Bx]
CreateUpVal     Upval[A] <- RC[Bx]
Copy            R[A] <- R[Bx]
Retain          retain R[A]
Release         release R[A]
Not             R[A] <- !RC[Bx]
Negate          R[A] <- -RC[Bx]
AddIF           R[A] <- RC[B] + RC[C]
SubIF           R[A] <- RC[B] - RC[C]
MulIF           R[A] <- RC[B] * RC[C]
DivIF           R[A] <- RC[B] / RC[C]
EqIFB           R[A] <- RC[B] == RC[C]
NeIFB           R[A] <- RC[B] != RC[C]
GtIF            R[A] <- RC[B] > RC[C]
GeIF            R[A] <- RC[B] >= RC[C]
LtIF            R[A] <- RC[B] < RC[C]
LeIF            R[A] <- RC[B] <= RC[C]
Cast            R[A] <- RC[B]
```

Notes:
* Instructions with IFO suffix operate on integers, floats or booleans
  respectively.
* R\[x] denotes register x.
* PC denotes the program counter.
* C\[x] denotes constant x.
* O\[x] denotes object constant x.
* RC\[x] denotes `x < 256 ? R[x] : C[x - 256]`.
* RO\[x] denotes `x < 256 ? R[x] : O[x - 256]`.
* Call calls the function in R\[B] / O\[B - 256] with A arguments
  starting from R\[C\]. The result is stored in R\[C\].
* CreateUpVal free's the UpValue at A and creates another one on that slot. It
  is intended to be used in loops to create a new UpValue for each iteration.

