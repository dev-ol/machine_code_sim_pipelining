          lw 0 1 x
          lw 0 2 y
          lw 0 6 e
start     add 2 6 2
          beq 1 2 start
          nand 3 1 5
done      halt
x         .fill 10
y         .fill 8
e         .fill 2