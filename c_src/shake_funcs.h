// -*- mode: c; tab-width: 8; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet

#include <decaf/shake.h>

/*
 * Erlang NIF functions
 */

#define SHA3_NIF(bits, bytes)	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_nif_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
	\
		if (argc != 1 || !enif_inspect_binary(env, argv[0], &in)) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		if (in.size <= MAX_PER_SLICE) {	\
			ERL_NIF_TERM out;	\
			unsigned char *buf = enif_make_new_binary(env, bytes, &out);	\
	\
			(void) decaf_sha3_##bits##_hash(buf, bytes, in.data, in.size);	\
	\
			return out;	\
		}	\
	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, sizeof(decaf_sha3_##bits##_ctx_t));	\
		struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(resource);	\
		(void) decaf_sha3_##bits##_init(sponge);	\
	\
		ERL_NIF_TERM newargv[4];	\
	\
		newargv[0] = argv[0];					/* In */	\
		newargv[1] = enif_make_ulong(env, MAX_PER_SLICE);	/* MaxPerSlice */	\
		newargv[2] = enif_make_ulong(env, 0);			/* Offset */	\
		newargv[3] = enif_make_resource(env, resource);		/* Sponge */	\
	\
		(void) enif_release_resource(resource);	\
	\
		return enif_schedule_nif(env, "sha3_" #bits, 0, libdecaf_sha3_##bits##_nif_4, 4, newargv);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_nif_4(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
		unsigned long max_per_slice;	\
		unsigned long offset;	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource;	\
	\
		if (argc != 4 || !enif_inspect_binary(env, argv[0], &in)	\
				|| !enif_get_ulong(env, argv[1], &max_per_slice)	\
				|| !enif_get_ulong(env, argv[2], &offset)	\
				|| !enif_get_resource(env, argv[3], resource_type, &resource)) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(resource);	\
	\
		struct timeval start;	\
		struct timeval stop;	\
		struct timeval slice;	\
		unsigned long end;	\
		unsigned long i;	\
		int percent;	\
		int total = 0;	\
	\
		end = offset + max_per_slice;	\
	\
		if (end > in.size) {	\
			end = in.size;	\
		}	\
	\
		i = offset;	\
	\
		while (i < in.size) {	\
			(void) gettimeofday(&start, NULL);	\
			(void) decaf_sha3_##bits##_update(sponge, (uint8_t *)(in.data) + i, end - i);	\
			i = end;	\
			if (i == in.size) {	\
				break;	\
			}	\
			(void) gettimeofday(&stop, NULL);	\
			/* determine how much of the timeslice was used */	\
			timersub(&stop, &start, &slice);	\
			percent = (int)((slice.tv_sec*1000000+slice.tv_usec)/10);	\
			total += percent;	\
			if (percent > 100) {	\
				percent = 100;	\
			} else if (percent == 0) {	\
				percent = 1;	\
			}	\
			if (enif_consume_timeslice(env, percent)) {	\
				/* the timeslice has been used up, so adjust our max_per_slice byte count based on the processing we've done, then reschedule to run again */	\
				max_per_slice = i - offset;	\
				if (total > 100) {	\
					int m = (int)(total/100);	\
					if (m == 1) {	\
						max_per_slice -= (unsigned long)(max_per_slice*(total-100)/100);	\
					} else {	\
						max_per_slice = (unsigned long)(max_per_slice/m);	\
					}	\
				}	\
				ERL_NIF_TERM newargv[4];	\
				newargv[0] = argv[0];					/* In */	\
				newargv[1] = enif_make_ulong(env, max_per_slice);	/* MaxPerSlice */	\
				newargv[2] = enif_make_ulong(env, i);			/* Offset */	\
				newargv[3] = argv[3];					/* Sponge */	\
				return enif_schedule_nif(env, "sha3_" #bits, 0, libdecaf_sha3_##bits##_nif_4, argc, newargv);	\
			}	\
			end += max_per_slice;	\
			if (end > in.size) {	\
				end = in.size;	\
			}	\
		}	\
	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, bytes, &out);	\
	\
		(void) decaf_sha3_##bits##_final(sponge, buf, bytes);	\
		(void) decaf_sha3_##bits##_destroy(sponge);	\
	\
		return out;	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_init_nif_0(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		if (argc != 0) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, sizeof(decaf_sha3_##bits##_ctx_t), &out);	\
		struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(buf);	\
	\
		(void) decaf_sha3_##bits##_init(sponge);	\
	\
		return enif_make_tuple2(env, ATOM_sha3_##bits, out);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_update_nif_2(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		int arity;	\
		const ERL_NIF_TERM *state;	\
		ErlNifBinary state_bin;	\
		ErlNifBinary in;	\
		\
		if (argc != 2 || !enif_get_tuple(env, argv[0], &arity, &state)	\
			|| arity != 2	\
			|| state[0] != ATOM_sha3_##bits	\
			|| !enif_inspect_binary(env, state[1], &state_bin)	\
			|| state_bin.size != sizeof(decaf_sha3_##bits##_ctx_t)	\
			|| !enif_inspect_binary(env, argv[1], &in)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		if (in.size <= MAX_PER_SLICE) {	\
			ERL_NIF_TERM out;	\
			unsigned char *buf = enif_make_new_binary(env, state_bin.size, &out);	\
			(void) memcpy(buf, state_bin.data, state_bin.size);	\
			struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(buf);	\
		\
			(void) decaf_sha3_##bits##_update(sponge, in.data, in.size);	\
		\
			return enif_make_tuple2(env, ATOM_sha3_##bits, out);	\
		}	\
		\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, state_bin.size);	\
		(void) memcpy(resource, state_bin.data, state_bin.size);	\
		\
		ERL_NIF_TERM newargv[4];	\
		\
		newargv[0] = argv[1];					/* In */	\
		newargv[1] = enif_make_ulong(env, MAX_PER_SLICE);	/* MaxPerSlice */	\
		newargv[2] = enif_make_ulong(env, 0);			/* Offset */	\
		newargv[3] = enif_make_resource(env, resource);		/* Sponge */	\
		\
		(void) enif_release_resource(resource);	\
		\
		return enif_schedule_nif(env, "sha3_" #bits "_update", 0, libdecaf_sha3_##bits##_update_nif_4, 4, newargv);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_update_nif_4(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
		unsigned long max_per_slice;	\
		unsigned long offset;	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource;	\
		\
		if (argc != 4 || !enif_inspect_binary(env, argv[0], &in)	\
				|| !enif_get_ulong(env, argv[1], &max_per_slice)	\
				|| !enif_get_ulong(env, argv[2], &offset)	\
				|| !enif_get_resource(env, argv[3], resource_type, &resource)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(resource);	\
		\
		struct timeval start;	\
		struct timeval stop;	\
		struct timeval slice;	\
		unsigned long end;	\
		unsigned long i;	\
		int percent;	\
		int total = 0;	\
		\
		end = offset + max_per_slice;	\
		\
		if (end > in.size) {	\
			end = in.size;	\
		}	\
		\
		i = offset;	\
		\
		while (i < in.size) {	\
			(void) gettimeofday(&start, NULL);	\
			(void) decaf_sha3_##bits##_update(sponge, (uint8_t *)(in.data) + i, end - i);	\
			i = end;	\
			if (i == in.size) {	\
				break;	\
			}	\
			(void) gettimeofday(&stop, NULL);	\
			/* determine how much of the timeslice was used */	\
			timersub(&stop, &start, &slice);	\
			percent = (int)((slice.tv_sec*1000000+slice.tv_usec)/10);	\
			total += percent;	\
			if (percent > 100) {	\
				percent = 100;	\
			} else if (percent == 0) {	\
				percent = 1;	\
			}	\
			if (enif_consume_timeslice(env, percent)) {	\
				/* the timeslice has been used up, so adjust our max_per_slice byte count based on the processing we've done, then reschedule to run again */	\
				max_per_slice = i - offset;	\
				if (total > 100) {	\
					int m = (int)(total/100);	\
					if (m == 1) {	\
						max_per_slice -= (unsigned long)(max_per_slice*(total-100)/100);	\
					} else {	\
						max_per_slice = (unsigned long)(max_per_slice/m);	\
					}	\
				}	\
				ERL_NIF_TERM newargv[4];	\
				newargv[0] = argv[0];					/* In */	\
				newargv[1] = enif_make_ulong(env, max_per_slice);	/* MaxPerSlice */	\
				newargv[2] = enif_make_ulong(env, i);			/* Offset */	\
				newargv[3] = argv[3];					/* Sponge */	\
				return enif_schedule_nif(env, "sha3_" #bits "_update", 0, libdecaf_sha3_##bits##_update_nif_4, argc, newargv);	\
			}	\
			end += max_per_slice;	\
			if (end > in.size) {	\
				end = in.size;	\
			}	\
		}	\
		\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, sizeof(decaf_sha3_##bits##_ctx_t), &out);	\
		(void) memcpy(buf, resource, sizeof(decaf_sha3_##bits##_ctx_t));	\
		\
		return enif_make_tuple2(env, ATOM_sha3_##bits, out);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_sha3_##bits##_final_nif_1(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		int arity;	\
		const ERL_NIF_TERM *state;	\
		ErlNifBinary state_bin;	\
		\
		if (argc != 1 || !enif_get_tuple(env, argv[0], &arity, &state)	\
			|| arity != 2	\
			|| state[0] != ATOM_sha3_##bits	\
			|| !enif_inspect_binary(env, state[1], &state_bin)	\
			|| state_bin.size != sizeof(decaf_sha3_##bits##_ctx_t)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, state_bin.size);	\
		(void) memcpy(resource, state_bin.data, state_bin.size);	\
		struct decaf_sha3_##bits##_ctx_s *sponge = (struct decaf_sha3_##bits##_ctx_s *)(resource);	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, bytes, &out);	\
		\
		(void) decaf_sha3_##bits##_final(sponge, buf, bytes);	\
		(void) decaf_sha3_##bits##_destroy(sponge);	\
		(void) enif_release_resource(resource);	\
		\
		return out;	\
	}

#define SHAKE_NIF(bits)	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_nif_2(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
		unsigned long outlen;	\
	\
		if (argc != 2 || !enif_inspect_binary(env, argv[0], &in)	\
				|| !enif_get_ulong(env, argv[1], &outlen)) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		if (in.size <= MAX_PER_SLICE) {	\
			ERL_NIF_TERM out;	\
			unsigned char *buf = enif_make_new_binary(env, outlen, &out);	\
	\
			(void) decaf_shake##bits##_hash(buf, outlen, in.data, in.size);	\
	\
			return out;	\
		}	\
	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, sizeof(decaf_shake##bits##_ctx_t));	\
		struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(resource);	\
		(void) decaf_shake##bits##_init(sponge);	\
	\
		ERL_NIF_TERM newargv[5];	\
	\
		newargv[0] = argv[0];					/* In */	\
		newargv[1] = argv[1];					/* Outlen */	\
		newargv[2] = enif_make_ulong(env, MAX_PER_SLICE);	/* MaxPerSlice */	\
		newargv[3] = enif_make_ulong(env, 0);			/* Offset */	\
		newargv[4] = enif_make_resource(env, resource);		/* Sponge */	\
	\
		(void) enif_release_resource(resource);	\
	\
		return enif_schedule_nif(env, "shake" #bits, 0, libdecaf_shake##bits##_nif_5, 5, newargv);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_nif_5(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
		unsigned long outlen;	\
		unsigned long max_per_slice;	\
		unsigned long offset;	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource;	\
	\
		if (argc != 5 || !enif_inspect_binary(env, argv[0], &in)	\
				|| !enif_get_ulong(env, argv[1], &outlen)	\
				|| !enif_get_ulong(env, argv[2], &max_per_slice)	\
				|| !enif_get_ulong(env, argv[3], &offset)	\
				|| !enif_get_resource(env, argv[4], resource_type, &resource)) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(resource);	\
	\
		struct timeval start;	\
		struct timeval stop;	\
		struct timeval slice;	\
		unsigned long end;	\
		unsigned long i;	\
		int percent;	\
		int total = 0;	\
	\
		end = offset + max_per_slice;	\
	\
		if (end > in.size) {	\
			end = in.size;	\
		}	\
	\
		i = offset;	\
	\
		while (i < in.size) {	\
			(void) gettimeofday(&start, NULL);	\
			(void) decaf_shake##bits##_update(sponge, (uint8_t *)(in.data) + i, end - i);	\
			i = end;	\
			if (i == in.size) {	\
				break;	\
			}	\
			(void) gettimeofday(&stop, NULL);	\
			/* determine how much of the timeslice was used */	\
			timersub(&stop, &start, &slice);	\
			percent = (int)((slice.tv_sec*1000000+slice.tv_usec)/10);	\
			total += percent;	\
			if (percent > 100) {	\
				percent = 100;	\
			} else if (percent == 0) {	\
				percent = 1;	\
			}	\
			if (enif_consume_timeslice(env, percent)) {	\
				/* the timeslice has been used up, so adjust our max_per_slice byte count based on the processing we've done, then reschedule to run again */	\
				max_per_slice = i - offset;	\
				if (total > 100) {	\
					int m = (int)(total/100);	\
					if (m == 1) {	\
						max_per_slice -= (unsigned long)(max_per_slice*(total-100)/100);	\
					} else {	\
						max_per_slice = (unsigned long)(max_per_slice/m);	\
					}	\
				}	\
				ERL_NIF_TERM newargv[5];	\
				newargv[0] = argv[0];					/* In */	\
				newargv[1] = argv[1];					/* Outlen */	\
				newargv[2] = enif_make_ulong(env, max_per_slice);	/* MaxPerSlice */	\
				newargv[3] = enif_make_ulong(env, i);			/* Offset */	\
				newargv[4] = argv[4];					/* Sponge */	\
				return enif_schedule_nif(env, "shake" #bits, 0, libdecaf_shake##bits##_nif_5, argc, newargv);	\
			}	\
			end += max_per_slice;	\
			if (end > in.size) {	\
				end = in.size;	\
			}	\
		}	\
	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, outlen, &out);	\
	\
		(void) decaf_shake##bits##_final(sponge, buf, outlen);	\
		(void) decaf_shake##bits##_destroy(sponge);	\
	\
		return out;	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_init_nif_0(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		if (argc != 0) {	\
			return enif_make_badarg(env);	\
		}	\
	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, sizeof(decaf_shake##bits##_ctx_t), &out);	\
		struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(buf);	\
	\
		(void) decaf_shake##bits##_init(sponge);	\
	\
		return enif_make_tuple2(env, ATOM_shake##bits, out);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_update_nif_2(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		int arity;	\
		const ERL_NIF_TERM *state;	\
		ErlNifBinary state_bin;	\
		ErlNifBinary in;	\
		\
		if (argc != 2 || !enif_get_tuple(env, argv[0], &arity, &state)	\
			|| arity != 2	\
			|| state[0] != ATOM_shake##bits	\
			|| !enif_inspect_binary(env, state[1], &state_bin)	\
			|| state_bin.size != sizeof(decaf_shake##bits##_ctx_t)	\
			|| !enif_inspect_binary(env, argv[1], &in)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		if (in.size <= MAX_PER_SLICE) {	\
			ERL_NIF_TERM out;	\
			unsigned char *buf = enif_make_new_binary(env, state_bin.size, &out);	\
			(void) memcpy(buf, state_bin.data, state_bin.size);	\
			struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(buf);	\
		\
			(void) decaf_shake##bits##_update(sponge, in.data, in.size);	\
		\
			return enif_make_tuple2(env, ATOM_shake##bits, out);	\
		}	\
		\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, state_bin.size);	\
		(void) memcpy(resource, state_bin.data, state_bin.size);	\
		\
		ERL_NIF_TERM newargv[4];	\
		\
		newargv[0] = argv[1];					/* In */	\
		newargv[1] = enif_make_ulong(env, MAX_PER_SLICE);	/* MaxPerSlice */	\
		newargv[2] = enif_make_ulong(env, 0);			/* Offset */	\
		newargv[3] = enif_make_resource(env, resource);		/* Sponge */	\
		\
		(void) enif_release_resource(resource);	\
		\
		return enif_schedule_nif(env, "shake" #bits "_update", 0, libdecaf_shake##bits##_update_nif_4, 4, newargv);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_update_nif_4(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		ErlNifBinary in;	\
		unsigned long max_per_slice;	\
		unsigned long offset;	\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource;	\
		\
		if (argc != 4 || !enif_inspect_binary(env, argv[0], &in)	\
				|| !enif_get_ulong(env, argv[1], &max_per_slice)	\
				|| !enif_get_ulong(env, argv[2], &offset)	\
				|| !enif_get_resource(env, argv[3], resource_type, &resource)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(resource);	\
		\
		struct timeval start;	\
		struct timeval stop;	\
		struct timeval slice;	\
		unsigned long end;	\
		unsigned long i;	\
		int percent;	\
		int total = 0;	\
		\
		end = offset + max_per_slice;	\
		\
		if (end > in.size) {	\
			end = in.size;	\
		}	\
		\
		i = offset;	\
		\
		while (i < in.size) {	\
			(void) gettimeofday(&start, NULL);	\
			(void) decaf_shake##bits##_update(sponge, (uint8_t *)(in.data) + i, end - i);	\
			i = end;	\
			if (i == in.size) {	\
				break;	\
			}	\
			(void) gettimeofday(&stop, NULL);	\
			/* determine how much of the timeslice was used */	\
			timersub(&stop, &start, &slice);	\
			percent = (int)((slice.tv_sec*1000000+slice.tv_usec)/10);	\
			total += percent;	\
			if (percent > 100) {	\
				percent = 100;	\
			} else if (percent == 0) {	\
				percent = 1;	\
			}	\
			if (enif_consume_timeslice(env, percent)) {	\
				/* the timeslice has been used up, so adjust our max_per_slice byte count based on the processing we've done, then reschedule to run again */	\
				max_per_slice = i - offset;	\
				if (total > 100) {	\
					int m = (int)(total/100);	\
					if (m == 1) {	\
						max_per_slice -= (unsigned long)(max_per_slice*(total-100)/100);	\
					} else {	\
						max_per_slice = (unsigned long)(max_per_slice/m);	\
					}	\
				}	\
				ERL_NIF_TERM newargv[4];	\
				newargv[0] = argv[0];					/* In */	\
				newargv[1] = enif_make_ulong(env, max_per_slice);	/* MaxPerSlice */	\
				newargv[2] = enif_make_ulong(env, i);			/* Offset */	\
				newargv[3] = argv[3];					/* Sponge */	\
				return enif_schedule_nif(env, "shake" #bits "_update", 0, libdecaf_shake##bits##_update_nif_4, argc, newargv);	\
			}	\
			end += max_per_slice;	\
			if (end > in.size) {	\
				end = in.size;	\
			}	\
		}	\
		\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, sizeof(decaf_shake##bits##_ctx_t), &out);	\
		(void) memcpy(buf, resource, sizeof(decaf_shake##bits##_ctx_t));	\
		\
		return enif_make_tuple2(env, ATOM_shake##bits, out);	\
	}	\
	\
	static ERL_NIF_TERM	\
	libdecaf_shake##bits##_final_nif_2(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])	\
	{	\
		int arity;	\
		const ERL_NIF_TERM *state;	\
		ErlNifBinary state_bin;	\
		unsigned long outlen;	\
		\
		if (argc != 2 || !enif_get_tuple(env, argv[0], &arity, &state)	\
			|| arity != 2	\
			|| state[0] != ATOM_shake##bits	\
			|| !enif_inspect_binary(env, state[1], &state_bin)	\
			|| state_bin.size != sizeof(decaf_shake##bits##_ctx_t)	\
			|| !enif_get_ulong(env, argv[1], &outlen)) {	\
			return enif_make_badarg(env);	\
		}	\
		\
		libdecaf_priv_data_t *priv_data = (libdecaf_priv_data_t *)(enif_priv_data(env));	\
		ErlNifResourceType *resource_type = priv_data->decaf_keccak_sponge;	\
		void *resource = enif_alloc_resource(resource_type, state_bin.size);	\
		(void) memcpy(resource, state_bin.data, state_bin.size);	\
		struct decaf_shake##bits##_ctx_s *sponge = (struct decaf_shake##bits##_ctx_s *)(resource);	\
		ERL_NIF_TERM out;	\
		unsigned char *buf = enif_make_new_binary(env, outlen, &out);	\
		\
		(void) decaf_shake##bits##_final(sponge, buf, outlen);	\
		(void) decaf_shake##bits##_destroy(sponge);	\
		(void) enif_release_resource(resource);	\
		\
		return out;	\
	}

SHA3_NIF(224, 28);
SHA3_NIF(256, 32);
SHA3_NIF(384, 48);
SHA3_NIF(512, 64);
SHAKE_NIF(128);
SHAKE_NIF(256);

#undef SHA3_NIF
#undef SHAKE_NIF
