          lw 0 1 x
          lw 0 2 y
start     add 2 2 3
          beq 1 2 start
          nand 3 1 5
done      halt
x         .fill 10
y         .fill 8