#include "../redismodule.h"
#include "../rmutil/util.h"
#include "../rmutil/strings.h"
#include <strings.h>
#include "../rmutil/test_util.h"
#include "./croaring.h"

#define malloc RedisModule_Alloc
#define calloc RedisModule_Calloc
#define realloc RedisModule_Realloc
#define free(ptr) RedisModule_Free(ptr)
#define strdup RedisModule_Strdup
#define aligned_malloc(ALIGNMENT, LEN) RedisModule_Alloc(LEN)

static RedisModuleType *RoaringType;

#define BITMAP_GET_OK 1
#define BITMAP_GET_ERR 2
#define BITMAP_GET_EMPTY 3

/**
 * Helper to get or create a bitmap
 *
 * Will reply with an error message and NULL if the key is the wrong type, otherwise creates or retrieves as necessary
 */
roaring_bitmap_t* get_create_bitmap(RedisModuleCtx *ctx, RedisModuleString *key_str, int perms) {
    roaring_bitmap_t *bitmap = NULL;

    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, key_str, perms);

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        bitmap = roaring_bitmap_create();
        RedisModule_ModuleTypeSetValue(key, RoaringType, bitmap);
    } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
        RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        bitmap = RedisModule_ModuleTypeGetValue(key);
    }

    return bitmap;
}

int get_bitmap(roaring_bitmap_t* bitmap, RedisModuleCtx *ctx, RedisModuleString *key_str, int perms) {
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, key_str, perms);
    bitmap = NULL;

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        return BITMAP_GET_EMPTY;
    } else if (RedisModule_ModuleTypeGetType(key) != RoaringType) {
        RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
        return BITMAP_GET_ERR;
    } else {
        bitmap = RedisModule_ModuleTypeGetValue(key);
        if (bitmap != NULL) {
            RedisModule_ReplyWithError(ctx, "??");
            return BITMAP_GET_ERR;
        } else {
            return BITMAP_GET_OK;
        }
    }
}

/**
 * ROARING.ADD <key> <value> ...
 *
 * Adds the series of values to the roaring bitmap defined by <key>
 */
int cmdAdd(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // argv format: [command, firstarg, secondarg, ...]
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    size_t count = (size_t)argc - 2;
    long long *to_add = calloc(count, sizeof(long long));

    // make sure that all the non-key args are integers
    for (int i = 0; i < count; i++) {
        if (RedisModule_StringToLongLong(argv[i + 2], &to_add[i]) != REDISMODULE_OK) {
            RedisModule_ReplyWithError(ctx, "Invalid argument, expects <key> <int>...");
            return REDISMODULE_ERR;
        }
    }

    roaring_bitmap_t* bitmap = get_create_bitmap(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    if (bitmap == NULL) {
        free(to_add);
        return REDISMODULE_ERR;
    }

    for (int i = 0; i < count; i++) {
        roaring_bitmap_add(bitmap, (uint32_t)to_add[i]);
    }

    RedisModule_ReplyWithLongLong(ctx, 1);
    RedisModule_ReplicateVerbatim(ctx);

    free(to_add);
    return REDISMODULE_OK;
}

/**
 * ROARING.REMOVE <key> <value> ...
 *
 * Removes elements from the bitmap
 */
int cmdRemove(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // TODO: Clean this up, is mostly duplicate code from above
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    size_t count = (size_t)argc - 2;
    long long *to_remove = calloc(count, sizeof(long long));
    for (int i = 0; i < count; i++) {
        if (RedisModule_StringToLongLong(argv[i + 2], &to_remove[i]) != REDISMODULE_OK) {
            RedisModule_ReplyWithError(ctx, "Invalid argument, expects <key> <int>...");
            return REDISMODULE_ERR;
        }
    }

    roaring_bitmap_t* bitmap = get_create_bitmap(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    if (bitmap == NULL) {
        free(to_remove);
        return REDISMODULE_ERR;
    }

    for (int i = 0; i < count; i++) {
        roaring_bitmap_remove(bitmap, (uint32_t)to_remove[i]);
    }

    RedisModule_ReplyWithLongLong(ctx, 1);
    RedisModule_ReplicateVerbatim(ctx);

    free(to_remove);
    return REDISMODULE_OK;
}

/**
 * ROARING.CARD <key>
 *
 * Returns cardinality of the roaring bitmap
 */
int cmdCard(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    char* format_err = "Expects format roaring.card included1 [included2 included3 ...] [^ excluded1 [excluded2] ...]";

    if (argc == 1) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    RedisModuleString* caret = RedisModule_CreateString(ctx, "^", 1);
    bool caret_found = false;

    roaring_bitmap_t* bitmap = roaring_bitmap_create();

    for (int i = 1; i < argc; i++) {
        if (RedisModule_StringCompare(argv[i], caret) == 0) {
RedisModule_Log(ctx, "info", "Found a caret");
            if (caret_found) {
                RedisModule_ReplyWithError(ctx, format_err);
                roaring_bitmap_free(bitmap);
                return REDISMODULE_ERR;
            } else {
                caret_found = true;
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
            return BITMAP_GET_ERR;
        }

        // otherwise, we have a set probably!
        arg_bitmap = RedisModule_ModuleTypeGetValue(key);
        if (!caret_found) {
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
    // TODO
    return (size_t)0;
}

void RoaringFree(void *value) {
    // TODO
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
