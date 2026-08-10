#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static int g_tests_run;
static int g_tests_failed;

#define RUN_TEST(test_func)                                                                          \
	do {                                                                                               \
		printf("Executando %s...", #test_func);                                                       \
		g_tests_run++;                                                                                  \
		const int failed_before = g_tests_failed;                                                      \
		test_func();                                                                                    \
		if (g_tests_failed == failed_before)                                                           \
			printf(" [PASS]\n");                                                                       \
		else                                                                                            \
			printf(" [FAIL]\n");                                                                       \
	} while (0)

#define ASSERT_TRUE(condition)                                                                       \
	do {                                                                                                \
		if (!(condition)) {                                                                             \
			printf("\n  FALHA: %s (Linha %d: %s)\n", __func__, __LINE__, #condition);                   \
			g_tests_failed++;                                                                             \
			return;                                                                                       \
		}                                                                                               \
	} while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ_INT(expected, actual)                                                              \
	do {                                                                                                \
		const long long exp_val = (long long)(expected);                                                \
		const long long act_val = (long long)(actual);                                                  \
		if (exp_val != act_val) {                                                                       \
			printf("\n  FALHA em %s (Linha %d): Esperado %lld, obtido %lld\n", __func__, __LINE__,      \
			       exp_val, act_val);                                                                     \
			g_tests_failed++;                                                                             \
			return;                                                                                       \
		}                                                                                               \
	} while (0)

#define ASSERT_FLOAT_NEAR(expected, actual, tolerance)                                               \
	do {                                                                                                \
		const float exp_val = (float)(expected);                                                        \
		const float act_val = (float)(actual);                                                          \
		if (!isfinite(act_val) || fabsf(exp_val - act_val) > (float)(tolerance)) {                     \
			printf("\n  FALHA em %s (Linha %d): Esperado %.4f, obtido %.4f (Tol: %.4f)\n", __func__,   \
			       __LINE__, (double)exp_val, (double)act_val, (double)(tolerance));                      \
			g_tests_failed++;                                                                             \
			return;                                                                                       \
		}                                                                                               \
	} while (0)

#define TEST_MAIN_END()                                                                              \
	do {                                                                                                \
		printf("\n========================================\n");                                          \
		printf("Resumo dos Testes: %d executados, %d falhas\n", g_tests_run, g_tests_failed);        \
		printf("========================================\n");                                          \
		return g_tests_failed > 0 ? 1 : 0;                                                             \
	} while (0)
