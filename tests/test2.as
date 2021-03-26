	lw	0	1	data1	
	lw	0	2	data2
     noop
     noop	
     noop	
     add  1    2    3	
     add  6    3    7	
     noop
     noop	
     noop	
     nand 3    1    5		
	lw	0	6	data1
	sw	6	6	11
	halt
data1	.fill	2
data2	.fill	3