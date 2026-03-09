10 REM Fibonacci Sequence
20 PRINT "First 20 Fibonacci numbers:"
30 LET A = 0
40 LET B = 1
50 FOR I = 1 TO 20
60 PRINT A
70 LET C = A + B
80 LET A = B
90 LET B = C
100 NEXT I
110 END
