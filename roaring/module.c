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
    uint32_t *values = calloc(count, sizeof(uint32_t));

    // make sure that all the non-key args are integers
    for (int i = 0; i < count; i++) {
        long long value;
        if (RedisModule_StringToLongLong(argv[i + 2], &value) != REDISMODULE_OK) {
            RedisModule_ReplyWithError(ctx, "Invalid argument, expects <key> <int>...");
            return REDISMODULE_ERR;
        } else {
            values[i] = (uint32_t)value;
        }
    }

    roaring_bitmap_t* bitmap = NULL;
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        if (!adding) {
            // empty key and we're removing, just return with no result
            RedisModule_ReplyWithLongLong(ctx, 1);
            return REDISMODULE_OK;
        }

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

    if (adding) {
        roaring_bitmap_add_many(bitmap, count, values);
    } else {
        // For whatever reason there is an add_many but not a remove_many
        for (int i = 0; i < count; i++) {
            roaring_bitmap_remove(bitmap, values[i]);
        }
    }

    // If we've removed and the bitmap is empty, get rid of the key
    if (!adding && roaring_bitmap_is_empty(bitmap)) {
        RedisModule_DeleteKey(key);
    }

    RedisModule_ReplyWithLongLong(ctx, 1);
    RedisModule_ReplicateVerbatim(ctx);

    free(values);
    return REDISMODULE_OK;
}

/**
 * ROARING.ADD <key> <value> ...
 *
 * Adds the series of values to the bitmap
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
    char* format_err = "Expects format roaring.card included1 [included2 included3 ...] [! excluded1 [excluded2] ...]";

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

/**
 * ROARING.MEMBERS <key>
 *
 * Returns all members of the bitmap
 */
int cmdMembers(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    // Fetch the bitmap under question
    roaring_bitmap_t* bitmap;
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        // If it's empty, return an empty array
        return RedisModule_ReplyWithArray(ctx, 0);
    } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
        RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
        return REDISMODULE_ERR;
    }

    bitmap = RedisModule_ModuleTypeGetValue(key);

    RedisModule_ReplyWithArray(ctx, roaring_bitmap_get_cardinality(bitmap));

    roaring_uint32_iterator_t* it = roaring_create_iterator(bitmap);
    while (it->has_value) {
        RedisModule_ReplyWithLongLong(ctx, it->current_value);
        roaring_advance_uint32_iterator(it);
    }
    roaring_free_uint32_iterator(it);

    return REDISMODULE_OK;
}

/**
 * ROARING.ISMEMBER <key> <value>
 *
 * Checks if the bitmap has the value
 */
int cmdIsMember(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    // Fetch the bitmap under question
    roaring_bitmap_t* bitmap;
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        // If it's empty, return false
        return RedisModule_ReplyWithLongLong(ctx, 0);
    } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
        RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
        return REDISMODULE_ERR;
    }

    bitmap = RedisModule_ModuleTypeGetValue(key);

    long long value = -1;
    RedisModule_StringToLongLong(argv[2], &value);
    bool contains = roaring_bitmap_contains(bitmap, (uint32_t)value);
    RedisModule_ReplyWithLongLong(ctx, contains);

    return REDISMODULE_OK;
}

void *RoaringRdbLoad(RedisModuleIO *rdb, int encver) {
    size_t* size = NULL;
    char *serialized = RedisModule_LoadStringBuffer(rdb, size);
    roaring_bitmap_t *bitmap = roaring_bitmap_deserialize(serialized);
    free(serialized);
    return bitmap;
}

void RoaringRdbSave(RedisModuleIO *rdb, void *value) {
    roaring_bitmap_t *bitmap = value;
    size_t size = roaring_bitmap_size_in_bytes(bitmap);
    char *serialized = malloc(size);

    roaring_bitmap_serialize(bitmap, serialized);

    RedisModule_SaveStringBuffer(rdb, serialized, size);

    free(serialized);
}

void RoaringAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
    // TODO - emit commands that will recreate the bitmap from the value to actual redis commands
    // exmaple: roaring.add keyname 104 174 1295 601828 55105 108417 ...
    // in batches of maybe 1000?
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
    RMUtil_RegisterReadCmd(ctx, "roaring.card", cmdCard);
    RMUtil_RegisterReadCmd(ctx, "roaring.members", cmdMembers);
    RMUtil_RegisterReadCmd(ctx, "roaring.ismember", cmdIsMember);

    return REDISMODULE_OK;
}
