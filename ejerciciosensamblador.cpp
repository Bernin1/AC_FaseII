/*
 * ============================================================
 * BENCHMARK FASE II - PARTE 2: Multiplicacion de Matrices SSE
 * Comparativa C vs Ensamblador x86 vs Ensamblador SSE
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <time.h>
#include <stdint.h>

 /* ── Parámetro principal ──────────────────────────────────── */
#define N 2000          /* Tamaño de la matriz N×N. N debe ser múltiplo de 4 para SSE */
#define TIRADAS 5       /* Número de ejecuciones para calcular la mediana */

/* ── PASO DEL MIEMBRO 2: Alineamiento estricto a 16 bytes ─── */
/* Vital para poder usar la instrucción rápida MOVAPS en SSE   */
#ifdef _MSC_VER
__declspec(align(16)) static int A[N][N];
__declspec(align(16)) static int B[N][N];
__declspec(align(16)) static int C[N][N];
__declspec(align(16)) static int C_ASM[N][N];
__declspec(align(16)) static int BT[N][N];
__declspec(align(16)) static int C_SSE[N][N];
#else
alignas(16) static int A[N][N];
alignas(16) static int B[N][N];
alignas(16) static int C[N][N];
alignas(16) static int C_ASM[N][N];
alignas(16) static int BT[N][N];
alignas(16) static int C_SSE[N][N];
#endif

/* ============================================================
 * 1. INICIALIZACIÓN Y TRANSPOSICIÓN
 * ============================================================ */
void inicializarMatrices() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (i + j) % 100;
            B[i][j] = (i - j) % 100;
        }
    }
}

void transponerB() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            BT[j][i] = B[i][j];
}

/* ============================================================
 * 2. MULTIPLICACIÓN EN C (Línea base)
 * ============================================================ */
void multiplicarMatricesC() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

/* ============================================================
 * 3. MULTIPLICACIÓN EN ENSAMBLADOR x86 (IA-32) - PARTE 1
 * ============================================================ */
void multiplicarMatricesASM() {
    __asm {
        push ebx
        push esi
        push edi
        push ebp

        xor esi, esi
        LOOP_I :
        cmp  esi, N
            jge  END_LOOP_I

            mov  ebx, esi
            imul ebx, N
            shl  ebx, 2

            xor edi, edi
            LOOP_J :
        cmp  edi, N
            jge  END_LOOP_J

            xor eax, eax
            xor ecx, ecx
            LOOP_K :
        cmp  ecx, N
            jge  END_LOOP_K

            mov  edx, ecx
            shl  edx, 2
            mov  edx, [A + ebx + edx]

            mov  ebp, ecx
            imul ebp, N
            add  ebp, edi
            shl  ebp, 2
            mov  ebp, [B + ebp]

            imul edx, ebp
            add  eax, edx

            inc  ecx
            jmp  LOOP_K
            END_LOOP_K :

        mov  edx, esi
            imul edx, N
            add  edx, edi
            shl  edx, 2
            mov[C_ASM + edx], eax

            inc  edi
            jmp  LOOP_J
            END_LOOP_J :
        inc  esi
            jmp  LOOP_I
            END_LOOP_I :

        pop  ebp
            pop  edi
            pop  esi
            pop  ebx
    }
}

/* ============================================================
 * 4. MULTIPLICACIÓN EN ENSAMBLADOR SSE (SIMD) - PARTE 2
 * ============================================================ */
void multiplicarMatricesASM_SSE() {
    __asm {
        push ebx
        push esi
        push edi
        push ebp

        xor esi, esi
        LOOP_I_SSE :
        cmp esi, N
            jge END_I_SSE

            xor edi, edi
            LOOP_J_SSE :
        cmp edi, N
            jge END_J_SSE

            xorps xmm0, xmm0
            xor ecx, ecx
            LOOP_K_SSE :
        cmp ecx, N
            jge END_K_SSE

            mov ebx, esi
            imul ebx, N
            add ebx, ecx
            shl ebx, 2
            // M2: Gracias al alineamiento a 16 bytes, usamos MOVAPS en lugar de MOVDQU (más rápido)
            movaps xmm1, [A + ebx]

            mov ebp, edi
            imul ebp, N
            add ebp, ecx
            shl ebp, 2
            movaps xmm2, [BT + ebp]

            cvtdq2ps xmm1, xmm1
            cvtdq2ps xmm2, xmm2
            mulps   xmm1, xmm2
            addps   xmm0, xmm1

            add ecx, 4
            jmp LOOP_K_SSE
            END_K_SSE :

        movaps xmm3, xmm0
            shufps xmm3, xmm3, 0x4E
            addps  xmm0, xmm3
            movaps xmm3, xmm0
            shufps xmm3, xmm3, 0xB1
            addps  xmm0, xmm3
            cvttss2si eax, xmm0

            mov ebx, esi
            imul ebx, N
            add ebx, edi
            shl ebx, 2
            mov[C_SSE + ebx], eax

            inc edi
            jmp LOOP_J_SSE
            END_J_SSE :
        inc esi
            jmp LOOP_I_SSE
            END_I_SSE :

        pop ebp
            pop edi
            pop esi
            pop ebx
    }
}

/* ============================================================
 * 5. VALIDACIÓN DE RESULTADOS
 * ============================================================ */
int validarResultados() {
    int pos[5][2] = { {0,0}, {0,N - 1}, {N / 2,N / 2}, {N - 1,0}, {N - 1,N - 1} };
    for (int p = 0; p < 5; p++) {
        int i = pos[p][0], j = pos[p][1];
        if (C[i][j] != C_ASM[i][j] || C[i][j] != C_SSE[i][j]) {
            return 0;
        }
    }
    return 1;
}

/* ============================================================
 * 6. FUNCIÓN AUXILIAR: CALCULAR MEDIANA (Aporte del M3)
 * ============================================================ */
double calcularMediana(std::vector<double>& tiempos) {
    std::sort(tiempos.begin(), tiempos.end());
    return tiempos[tiempos.size() / 2]; // Devuelve el valor central
}

/* ============================================================
 * MAIN: BANCO DE PRUEBAS AUTOMATIZADO
 * ============================================================ */
int main() {
    std::cout << "==========================================================\n";
    std::cout << "  BENCHMARK: Multiplicacion de Matrices " << N << "x" << N << "\n";
    std::cout << "  Modo: Evaluacion Rigurosa (" << TIRADAS << " tiradas / Mediana)\n";
    std::cout << "==========================================================\n\n";

    std::cout << "[1/3] Inicializando matrices y transponiendo B ... ";
    inicializarMatrices();
    transponerB();
    std::cout << "OK\n";

    std::vector<double> tiemposC(TIRADAS), tiemposASM(TIRADAS), tiemposSSE(TIRADAS);

    std::cout << "[2/3] Ejecutando Benchmark (" << TIRADAS << " iteraciones). Por favor, espere...\n";

    for (int t = 0; t < TIRADAS; t++) {
        clock_t inicio, fin;

        // 1. Test C
        inicio = clock();
        multiplicarMatricesC();
        fin = clock();
        tiemposC[t] = (double)(fin - inicio) / CLOCKS_PER_SEC;

        // 2. Test ASM x86
        inicio = clock();
        multiplicarMatricesASM();
        fin = clock();
        tiemposASM[t] = (double)(fin - inicio) / CLOCKS_PER_SEC;

        // 3. Test ASM SSE
        inicio = clock();
        multiplicarMatricesASM_SSE();
        fin = clock();
        tiemposSSE[t] = (double)(fin - inicio) / CLOCKS_PER_SEC;

        std::cout << "      -> Tirada " << t + 1 << " completada.\n";
    }

    std::cout << "[3/3] Validando integridad de los datos ... ";
    int valido = validarResultados();
    std::cout << (valido ? "CORRECTO\n\n" : "FALLO\n\n");

    if (!valido) {
        std::cout << "¡ATENCION! Los calculos no coinciden. Revisa el codigo.\n";
        return -1;
    }

    /* ── CÁLCULO DE MEDIANAS Y SPEEDUP ─────────────────────── */
    double medianaC = calcularMediana(tiemposC);
    double medianaASM = calcularMediana(tiemposASM);
    double medianaSSE = calcularMediana(tiemposSSE);

    double speedupASM = (medianaASM > 0.0) ? (medianaC / medianaASM) : 0.0;
    double speedupSSE = (medianaSSE > 0.0) ? (medianaC / medianaSSE) : 0.0;
    double speedupPuro = (medianaSSE > 0.0) ? (medianaASM / medianaSSE) : 0.0; // SSE vs ASM

    /* ── INFORME FINAL PARA EL PROFESOR ────────────────────── */
    printf("----------------------------------------------------------\n");
    printf("  RESULTADOS FINALES (MEDIANAS)\n");
    printf("----------------------------------------------------------\n");
    printf("  Tiempo Medio C          : %.4f s\n", medianaC);
    printf("  Tiempo Medio ASM (x86)  : %.4f s\n", medianaASM);
    printf("  Tiempo Medio ASM (SSE)  : %.4f s\n\n", medianaSSE);

    printf("  [M5] Analisis de Speedup:\n");
    printf("  - Ganancia ASM vs C     : %.2fx\n", speedupASM);
    printf("  - Ganancia SSE vs C     : %.2fx\n", speedupSSE);
    printf("  - Ganancia pura SSE/ASM : %.2fx (Paralelismo SIMD)\n", speedupPuro);

    printf("\n==========================================================\n");
    printf("  DATOS LISTOS PARA LA MEMORIA GRUPAL\n");
    printf("==========================================================\n");

    return 0;
}