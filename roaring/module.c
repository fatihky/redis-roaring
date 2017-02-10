#include "../redismodule.h"
#include "../rmutil/util.h"
#include "../rmutil/strings.h"
#include "../rmutil/test_util.h"
#include "./croaring.h"

#define malloc RedisModule_Alloc
#define calloc RedisModule_Calloc
#define realloc RedisModule_Realloc
#define free(ptr) RedisModule_Free(ptr)
#define strdup RedisModule_Strdup
#define aligned_malloc(ALIGNMENT, LEN) RedisModule_Alloc(LEN)

static RedisModuleType *RoaringType;

/**
 * Since add and remove are so similar, unify them in this one path.
 *
 * If arg `adding` is true, then adds. Otherwise removes.
 */
int _cmdAddOrRemove(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, bool adding) {
    // argv format: [command, firstarg, secondarg, ...]
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    size_t count = (size_t)argc - 2;
    long long *values = calloc(count, sizeof(long long));

    // make sure that all the non-key args are integers
    for (int i = 0; i < count; i++) {
        if (RedisModule_StringToLongLong(argv[i + 2], &values[i]) != REDISMODULE_OK) {
            RedisModule_ReplyWithError(ctx, "Invalid argument, expects <key> <int>...");
            return REDISMODULE_ERR;
        }
    }

    roaring_bitmap_t* bitmap = NULL;
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        // If the bitmap doesn't exist, create it and store it's reference
        bitmap = roaring_bitmap_create();
        RedisModule_ModuleTypeSetValue(key, RoaringType, bitmap);
    } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
        // If it's the wrong type, quit out!
        RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
        free(values);
        return REDISMODULE_ERR;
    } else {
        // Otherwise we have a valid bitmap key - grab it
        bitmap = RedisModule_ModuleTypeGetValue(key);
    }

    for (int i = 0; i < count; i++) {
        if (adding) {
            roaring_bitmap_add(bitmap, (uint32_t)values[i]);
        } else {
            roaring_bitmap_remove(bitmap, (uint32_t)values[i]);
        }
    }

    RedisModule_ReplyWithLongLong(ctx, 1);
    RedisModule_ReplicateVerbatim(ctx);

    free(values);
    return REDISMODULE_OK;
}

/**
 * ROARING.ADD <key> <value> ...
 *
 * Adds the series of values to the roaring bitmap defined by <key>
 */
int cmdAdd(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    return _cmdAddOrRemove(ctx, argv, argc, true);
}

/**
 * ROARING.REMOVE <key> <value> ...
 *
 * Removes elements from the bitmap
 */
int cmdRemove(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    return _cmdAddOrRemove(ctx, argv, argc, false);
}

/**
 * ROARING.CARD <inc1> [<inc2> ...] [! <exc1> [<exc2> ...]]
 *
 * Returns cardinality of the roaring bitmaps, excluding each after the bang (!)
 */
int cmdCard(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    char* format_err = "Expects format roaring.card included1 [included2 included3 ...] [^ excluded1 [excluded2] ...]";

    if (argc == 1) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    RedisModuleString* bang = RedisModule_CreateString(ctx, "!", 1);
    bool bang_found = false;

    roaring_bitmap_t* bitmap = roaring_bitmap_create();

    for (int i = 1; i < argc; i++) {
        if (RedisModule_StringCompare(argv[i], bang) == 0) {
            if (bang_found) {
                RedisModule_ReplyWithError(ctx, format_err);
                roaring_bitmap_free(bitmap);
                return REDISMODULE_ERR;
            } else {
                bang_found = true;
                continue;
            }
        }

        // Fetch the bitmap under question
        roaring_bitmap_t* arg_bitmap;
        RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[i], REDISMODULE_READ);
        if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
            // If there is nothing to include or exclude, just continue on
            continue;
        } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
            RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
            roaring_bitmap_free(bitmap);
            return REDISMODULE_ERR;
        }

        // otherwise, we have a set probably!
        arg_bitmap = RedisModule_ModuleTypeGetValue(key);
        if (!bang_found) {
            roaring_bitmap_or_inplace(bitmap, arg_bitmap);
        } else {
            roaring_bitmap_andnot_inplace(bitmap, arg_bitmap);
        }
    }

    RedisModule_ReplyWithLongLong(ctx, roaring_bitmap_get_cardinality(bitmap));
    roaring_bitmap_free(bitmap);

    return REDISMODULE_OK;
}

void *RoaringRdbLoad(RedisModuleIO *rdb, int encver) {
    // TODO
    return NULL;
}

void RoaringRdbSave(RedisModuleIO *rdb, void *value) {
    // TODO
}

void RoaringAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
    // TODO
}

size_t RoaringMemUsage(const void *value) {
    roaring_statistics_t stats;
    roaring_bitmap_statistics(value, &stats);
    return stats.n_bytes_array_containers + stats.n_bytes_bitset_containers + stats.n_bytes_run_containers;
}

void RoaringFree(void *value) {
   roaring_bitmap_free(value);
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {

    // Register the module itself
    if (RedisModule_Init(ctx, "roaring", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    RedisModuleTypeMethods tm = {
            .version = REDISMODULE_TYPE_METHOD_VERSION,
            .rdb_load = RoaringRdbLoad,
            .rdb_save = RoaringRdbSave,
            .aof_rewrite = RoaringAofRewrite,
            .mem_usage = RoaringMemUsage,
            .free = RoaringFree
    };

    RoaringType = RedisModule_CreateDataType(ctx, "c_roaring", 0, &tm);
    if (RoaringType == NULL) return REDISMODULE_ERR;

    // register commands
    RMUtil_RegisterWriteCmd(ctx, "roaring.add", cmdAdd);
    RMUtil_RegisterWriteCmd(ctx, "roaring.remove", cmdRemove);
    RMUtil_RegisterWriteCmd(ctx, "roaring.card", cmdCard);

    return REDISMODULE_OK;
}
