10 REM Number Guessing Game
20 PRINT "I'm thinking of a number between 1 and 100."
30 LET SECRET = INT(RND(0) * 100) + 1
40 LET TRIES = 0
50 INPUT "Your guess"; G
60 LET TRIES = TRIES + 1
70 IF G = SECRET THEN 110
80 IF G < SECRET THEN PRINT "Too low!"
90 IF G > SECRET THEN PRINT "Too high!"
100 GOTO 50
110 PRINT "You got it in "; TRIES; " tries!"
120 END
