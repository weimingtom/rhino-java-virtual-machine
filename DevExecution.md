This is the simplified structure of the execution code.
```
o-> [1:read opcode] <----------------------o
|        |                                 |
|   [2:execute opcode]                     |
|        |                                 |
|        |                                 |
|   [3:return opcode]--->[4:exit]          |
|        |                                 |
|        |                                 |
|        |                                 |
|        |                                 |
|        |                                 |
|        |                                 |
o--------o---[5:did error happen?]         |
                         |                 |
                         |                 |
  [6:try/catch block?]<--o                 |
            |                              |
            o-->[7:reset stack]-->[8:jump to handler]
```
_(1)_ and _(2)_ is where we read the first byte. Then, a switch statement is used to find the _opcode_. From there we read additional bytes which are determined by the instruction. Also local variables and the stack may be manipulated depending on the instruction

Execution follows this general path. It all happens, at the time of this writting, in one C procedure called _jvm\_ExecuteObjectMethod_. When an
error is thrown either by an _opcode_ or by the runtime it gets checked
for before the next instruction is fetched. If the error has a try and
a catch block for the correct exception object then the stack will be
reset and execution will start at the place specified for the
exception handler.