/*
 * This software is Copyright (c) 2015 Sayantan Datta <std2048 at gmail dot com>
 * and it is hereby released to the general public under the following terms:
 * Redistribution and use in source and binary forms, with or without modification, are permitted.
 * Based on Solar Designer implementation of DES_fmt.c in jtr-v1.7.9
 */

#if HAVE_OPENCL

#if FMT_EXTERNS_H
extern struct fmt_main fmt_opencl_lm;
#elif FMT_REGISTERS_H
john_register_one(&fmt_opencl_lm);
#else

#include <string.h>

#include "opencl_lm.h"
#include "arch.h"
#include "common.h"
#include "formats.h"
#include "config.h"
#include "opencl_lm_hst_dev_shared.h"
#include "memdbg.h"

#define FORMAT_NAME			""

#define BENCHMARK_COMMENT		""
#define BENCHMARK_LENGTH		-1

#define CIPHERTEXT_LENGTH		32

#define LM_EMPTY			"aad3b435b51404ee"

static struct fmt_tests tests[] = {
	{"$LM$a9c604d244c4e99d", "AAAAAA"},
	{"$LM$cbc501a4d2227783", "AAAAAAA"},
	{"$LM$3466c2b0487fe39a", "CRACKPO"},
	{"$LM$dbc5e5cba8028091", "IMPUNIT"},
	{LM_EMPTY LM_EMPTY, ""},
	{"$LM$73cc402bd3e79175", "SCLEROS"},
	{"$LM$5ecd9236d21095ce", "YOKOHAM"},
	{"$LM$A5E6066DE61C3E35", "ZZZZZZZ"}, /* uppercase encoding */
	{"$LM$1FB363feB834C12D", "ZZZZZZ"}, /* mixed case encoding */
	{NULL}
};

#define ALGORITHM_NAME			LM_OPENCL_ALGORITHM_NAME

#define BINARY_SIZE			(2 * sizeof(WORD))
#define BINARY_ALIGN			sizeof(WORD)
#define SALT_SIZE			0
#define SALT_ALIGN			1


void (*opencl_lm_init_global_variables)(void);

static void init(struct fmt_main *pFmt)
{
	opencl_prepare_dev(gpu_id);
	opencl_lm_b_register_functions(pFmt);
	opencl_lm_init_global_variables();
}

static char *prepare(char *fields[10], struct fmt_main *self)
{
	if (fields[2] && strlen(fields[2]) == 32)
		return fields[2];
	return fields[1];
}

static int valid(char *ciphertext, struct fmt_main *self)
{
	char *pos;
	char lower[CIPHERTEXT_LENGTH - 16 + 1];

	for (pos = ciphertext; atoi16[ARCH_INDEX(*pos)] != 0x7F; pos++);
	if (!*pos && pos - ciphertext == CIPHERTEXT_LENGTH) {
		strcpy(lower, &ciphertext[16]);
		strlwr(lower);
		if (strcmp(lower, LM_EMPTY))
			return 2;
		else
			return 1;
	}

	if (strncmp(ciphertext, "$LM$", 4)) return 0;

	for (pos = &ciphertext[4]; atoi16[ARCH_INDEX(*pos)] != 0x7F; pos++);
	if (*pos || pos - ciphertext != 20) return 0;

	return 1;
}

static char *split(char *ciphertext, int index, struct fmt_main *self)
{
	static char out[21];

/* We don't just "return ciphertext" for already split hashes since we may
 * need to convert hashes stored by older versions of John to all-lowercase. */
	if (!strncmp(ciphertext, "$LM$", 4))
		ciphertext += 4;

	out[0] = '$';
	out[1] = 'L';
	out[2] = 'M';
	out[3] = '$';

	if (index)
		memcpy(&out[4], &ciphertext[16], 16);
	else
		memcpy(&out[4], ciphertext, 16);

	out[20] = 0;

	strlwr(&out[4]);

	return out;
}

static void *binary(char *ciphertext)
{
	return opencl_lm_get_binary(ciphertext + 4);
}

static char *source(char *source, void *binary)
{
	return split(opencl_lm_get_source(binary), 0, NULL);
}

static int binary_hash_0(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_0;
}

static int binary_hash_1(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_1;
}

static int binary_hash_2(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_2;
}

static int binary_hash_3(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_3;
}

static int binary_hash_4(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_4;
}

static int binary_hash_5(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_5;
}

static int binary_hash_6(void *binary)
{
	return *(unsigned WORD *)binary & PH_MASK_6;
}

#define get_hash_0 opencl_lm_get_hash_0
#define get_hash_1 opencl_lm_get_hash_1
#define get_hash_2 opencl_lm_get_hash_2
#define get_hash_3 opencl_lm_get_hash_3
#define get_hash_4 opencl_lm_get_hash_4
#define get_hash_5 opencl_lm_get_hash_5
#define get_hash_6 opencl_lm_get_hash_6

static int cmp_all(WORD *binary, int count)
{
	return 1;
}

struct fmt_main fmt_opencl_lm = {
	{
		FORMAT_LABEL,
		FORMAT_NAME,
		ALGORITHM_NAME,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
		FMT_8_BIT | FMT_BS | FMT_TRUNC | FMT_SPLIT_UNIFIES_CASE | FMT_REMOVE,
		{ NULL },
		tests
	}, {
		init,
		NULL,
		NULL,
		prepare,
		valid,
		split,
		binary,
		fmt_default_salt,
		{ NULL },
		source,
		{
			binary_hash_0,
			binary_hash_1,
			binary_hash_2,
			binary_hash_3,
			binary_hash_4,
			binary_hash_5,
			binary_hash_6
		},
		fmt_default_salt_hash,
		NULL,
		fmt_default_set_salt,
		opencl_lm_set_key,
		NULL,
		fmt_default_clear_keys,
		NULL,
		{
			get_hash_0,
			get_hash_1,
			get_hash_2,
			get_hash_3,
			get_hash_4,
			get_hash_5,
			get_hash_6
		},
		(int (*)(void *, int))cmp_all,
		NULL,
		NULL
	}
};
#endif /* plugin stanza */

#endif /* #if HAVE_OPENCL */
