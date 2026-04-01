5 OPTION GPU BASE 0
10 REM === 2D Heat Diffusion with GPU and MPI ===
20 REM Simulates heat spreading from a hot top wall (T=100)
30 REM to a cold bottom wall (T=0) on a 32x32 grid.
40 REM Run: mpirun -n 4 rocBAS examples/heat.bas
100 MPI INIT
110 LET R = MPI RANK
120 LET S = MPI SIZE
130 REM --- Grid parameters ---
140 LET NX = 32
150 LET NY = 32
160 LET LR = NX / S
170 LET LROWS = LR + 2
180 LET SZ = LROWS * NY
190 LET NT = 1000
195 LET ALPHA = 0.1
200 REM --- Row index bounds (host 1-based) ---
210 LET B1 = NY + 1
220 LET E1 = 2 * NY
230 LET BL = LR * NY + 1
240 LET EL = (LR + 1) * NY
250 LET BG = (LR + 1) * NY + 1
260 LET EG = (LR + 2) * NY
300 REM --- Allocate host arrays ---
310 DIM U(SZ), U2(SZ)
400 REM --- Initialize: all zeros ---
410 FOR I = 1 TO SZ
420 LET U(I) = 0
430 NEXT I
440 REM --- Rank 0: set top ghost row (hot boundary = 100) ---
450 IF R > 0 THEN GOTO 490
460 FOR J = 1 TO NY
470 LET U(J) = 100
480 NEXT J
490 REM --- Allocate GPU arrays and upload initial state ---
500 GPU DIM GU(SZ), GU2(SZ)
510 GPU COPY U TO GU
600 REM --- Define heat stencil kernel ---
610 GPU KERNEL HEAT(UOLD, UNEW, W, H, COEFF, N)
620 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
630 LET ROW = INT(I / W)
640 LET COL = I - ROW * W
650 IF I < N THEN IF ROW > 0 THEN IF ROW < H - 1 THEN IF COL > 0 THEN IF COL < W - 1 THEN LET UNEW(I) = UOLD(I) + COEFF * (UOLD(I - W) + UOLD(I + W) + UOLD(I - 1) + UOLD(I + 1) - 4 * UOLD(I))
660 END KERNEL
700 REM === Time stepping loop ===
710 FOR T = 1 TO NT
720 REM --- Compute stencil on GPU ---
730 GPU GOSUB HEAT(GU, GU2, NY, LROWS, ALPHA, SZ) WITH 2 BLOCKS OF 256
740 REM --- Copy new values to host for MPI exchange ---
750 GPU COPY GU2 TO U2
760 REM --- Exchange: send last real row DOWN (tag 1) ---
770 IF R >= S - 1 THEN GOTO 790
780 MPI SEND U2(BL) THRU U2(EL) TO R + 1 TAG 1
790 REM --- Recv top ghost from above (tag 1) ---
800 IF R <= 0 THEN GOTO 820
810 MPI RECV U2(1) THRU U2(NY) FROM R - 1 TAG 1
820 REM --- Exchange: send first real row UP (tag 2) ---
830 IF R <= 0 THEN GOTO 850
840 MPI SEND U2(B1) THRU U2(E1) TO R - 1 TAG 2
850 REM --- Recv bottom ghost from below (tag 2) ---
860 IF R >= S - 1 THEN GOTO 880
870 MPI RECV U2(BG) THRU U2(EG) FROM R + 1 TAG 2
880 MPI BARRIER
890 REM --- Re-apply rank 0 top boundary (hot wall) ---
900 IF R > 0 THEN GOTO 940
910 FOR J = 1 TO NY
920 LET U2(J) = 100
930 NEXT J
940 REM --- Upload corrected state for next timestep ---
950 GPU COPY U2 TO GU
960 NEXT T
1000 REM === Final output ===
1010 GPU COPY GU TO U
1020 MPI BARRIER
1030 IF R > 0 THEN GOTO 1060
1040 PRINT "=== 2D Heat Diffusion ("; NX; "x"; NY; ", "; NT; " steps, alpha="; ALPHA; ") ==="
1060 MPI BARRIER
1070 LET PR = 0
1080 IF R <> PR THEN GOTO 1110
1090 PRINT "Rank "; R; ": top_mid="; U(NY + INT(NY / 2)); " bot_mid="; U(LR * NY + INT(NY / 2))
1110 MPI BARRIER
1120 LET PR = PR + 1
1130 IF PR < S THEN GOTO 1080
1200 REM --- Cleanup ---
1210 GPU FREE GU
1220 GPU FREE GU2
1230 MPI FINALIZE
1240 END
