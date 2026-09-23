#include "pc_pointer_token.h"

#include <stdatomic.h>
#include <stddef.h>

#define PC_POINTER_TOKEN_TABLE_SIZE (PC_POINTER_TOKEN_CAPACITY * 2u)
#define PC_POINTER_TOKEN_DOMAIN_SHIFT 26u
#define PC_POINTER_TOKEN_DOMAIN_MASK UINT32_C(0x0C000000)
#define PC_POINTER_TOKEN_SEQUENCE_MASK UINT32_C(0x03FFFFFF)
#define PC_POINTER_TOKEN_TOMBSTONE UINT32_MAX

typedef struct pc_pointer_token_entry {
    uint32_t token;
    uintptr_t pointer;
} pc_pointer_token_entry;

static pc_pointer_token_entry s_pointer_tokens[PC_POINTER_TOKEN_TABLE_SIZE];
static uint32_t s_next_sequence[PC_POINTER_TOKEN_DOMAIN_ACMD + 1u] = {0u, 1u, 1u};
static uint32_t s_active_count[PC_POINTER_TOKEN_DOMAIN_ACMD + 1u];
static atomic_flag s_pointer_token_lock = ATOMIC_FLAG_INIT;

static int pc_pointer_token_domain_valid(pc_pointer_token_domain domain) {
    return domain == PC_POINTER_TOKEN_DOMAIN_GBI || domain == PC_POINTER_TOKEN_DOMAIN_ACMD;
}

static pc_pointer_token_domain pc_pointer_token_domain_from_token(uint32_t token) {
    return (pc_pointer_token_domain)((token & PC_POINTER_TOKEN_DOMAIN_MASK) >> PC_POINTER_TOKEN_DOMAIN_SHIFT);
}

static size_t pc_pointer_token_hash(uint32_t token) {
    return (size_t)((token * UINT32_C(2654435761)) & (PC_POINTER_TOKEN_TABLE_SIZE - 1u));
}

static void pc_pointer_token_lock(void) {
    while (atomic_flag_test_and_set_explicit(&s_pointer_token_lock, memory_order_acquire)) {
    }
}

static void pc_pointer_token_unlock(void) {
    atomic_flag_clear_explicit(&s_pointer_token_lock, memory_order_release);
}

static size_t pc_pointer_token_find(uint32_t token) {
    size_t index = pc_pointer_token_hash(token);
    size_t i;

    for (i = 0; i < PC_POINTER_TOKEN_TABLE_SIZE; i++) {
        uint32_t candidate = s_pointer_tokens[index].token;

        if (candidate == token) {
            return index;
        }
        if (candidate == 0u) {
            break;
        }
        index = (index + 1u) & (PC_POINTER_TOKEN_TABLE_SIZE - 1u);
    }

    return PC_POINTER_TOKEN_TABLE_SIZE;
}

int pc_pointer_token_is_token(uint32_t value) {
    return (value & PC_POINTER_TOKEN_NAMESPACE_MASK) == PC_POINTER_TOKEN_NAMESPACE;
}

const char* pc_pointer_token_result_name(pc_pointer_token_result result) {
    switch (result) {
        case PC_POINTER_TOKEN_OK:
            return "ok";
        case PC_POINTER_TOKEN_NOT_TOKEN:
            return "not a token";
        case PC_POINTER_TOKEN_INVALID_DOMAIN:
            return "invalid domain";
        case PC_POINTER_TOKEN_WRONG_DOMAIN:
            return "wrong domain";
        case PC_POINTER_TOKEN_STALE:
            return "stale token";
        case PC_POINTER_TOKEN_EXHAUSTED:
            return "token registry exhausted";
        default:
            return "unknown token error";
    }
}

pc_pointer_token_result pc_pointer_token_pack(pc_pointer_token_domain domain, uintptr_t pointer,
                                              uint32_t* token_out) {
    size_t first_tombstone = PC_POINTER_TOKEN_TABLE_SIZE;
    size_t index;
    size_t i;
    uint32_t sequence;
    uint32_t token;

    if (!pc_pointer_token_domain_valid(domain) || token_out == NULL) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }

    pc_pointer_token_lock();
    if (s_active_count[domain] >= PC_POINTER_TOKEN_CAPACITY) {
        pc_pointer_token_unlock();
        return PC_POINTER_TOKEN_EXHAUSTED;
    }

    for (i = 0; i < PC_POINTER_TOKEN_TABLE_SIZE; i++) {
        if (s_pointer_tokens[i].token != 0u && s_pointer_tokens[i].token != PC_POINTER_TOKEN_TOMBSTONE &&
            pc_pointer_token_domain_from_token(s_pointer_tokens[i].token) == domain &&
            s_pointer_tokens[i].pointer == pointer) {
            *token_out = s_pointer_tokens[i].token;
            pc_pointer_token_unlock();
            return PC_POINTER_TOKEN_OK;
        }
    }

    sequence = s_next_sequence[domain];
    if (sequence == 0u || sequence > PC_POINTER_TOKEN_SEQUENCE_MASK) {
        pc_pointer_token_unlock();
        return PC_POINTER_TOKEN_EXHAUSTED;
    }
    s_next_sequence[domain]++;
    token = PC_POINTER_TOKEN_NAMESPACE | ((uint32_t)domain << PC_POINTER_TOKEN_DOMAIN_SHIFT) | sequence;
    index = pc_pointer_token_hash(token);

    for (i = 0; i < PC_POINTER_TOKEN_TABLE_SIZE; i++) {
        if (s_pointer_tokens[index].token == PC_POINTER_TOKEN_TOMBSTONE &&
            first_tombstone == PC_POINTER_TOKEN_TABLE_SIZE) {
            first_tombstone = index;
        } else if (s_pointer_tokens[index].token == 0u) {
            if (first_tombstone != PC_POINTER_TOKEN_TABLE_SIZE) {
                index = first_tombstone;
            }
            s_pointer_tokens[index].token = token;
            s_pointer_tokens[index].pointer = pointer;
            s_active_count[domain]++;
            *token_out = token;
            pc_pointer_token_unlock();
            return PC_POINTER_TOKEN_OK;
        }
        index = (index + 1u) & (PC_POINTER_TOKEN_TABLE_SIZE - 1u);
    }

    if (first_tombstone != PC_POINTER_TOKEN_TABLE_SIZE) {
        s_pointer_tokens[first_tombstone].token = token;
        s_pointer_tokens[first_tombstone].pointer = pointer;
        s_active_count[domain]++;
        *token_out = token;
        pc_pointer_token_unlock();
        return PC_POINTER_TOKEN_OK;
    }

    pc_pointer_token_unlock();
    return PC_POINTER_TOKEN_EXHAUSTED;
}

pc_pointer_token_result pc_pointer_token_resolve(pc_pointer_token_domain domain, uint32_t token,
                                                 uintptr_t* pointer_out) {
    size_t index;

    if (!pc_pointer_token_domain_valid(domain) || pointer_out == NULL) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }
    if (!pc_pointer_token_is_token(token)) {
        return PC_POINTER_TOKEN_NOT_TOKEN;
    }
    if (!pc_pointer_token_domain_valid(pc_pointer_token_domain_from_token(token))) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }
    if (pc_pointer_token_domain_from_token(token) != domain) {
        return PC_POINTER_TOKEN_WRONG_DOMAIN;
    }

    pc_pointer_token_lock();
    index = pc_pointer_token_find(token);
    if (index == PC_POINTER_TOKEN_TABLE_SIZE) {
        pc_pointer_token_unlock();
        return PC_POINTER_TOKEN_STALE;
    }
    *pointer_out = s_pointer_tokens[index].pointer;
    pc_pointer_token_unlock();
    return PC_POINTER_TOKEN_OK;
}

pc_pointer_token_result pc_pointer_token_release(pc_pointer_token_domain domain, uint32_t token) {
    size_t index;
    pc_pointer_token_domain token_domain;

    if (!pc_pointer_token_domain_valid(domain)) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }
    if (!pc_pointer_token_is_token(token)) {
        return PC_POINTER_TOKEN_NOT_TOKEN;
    }
    token_domain = pc_pointer_token_domain_from_token(token);
    if (!pc_pointer_token_domain_valid(token_domain)) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }
    if (token_domain != domain) {
        return PC_POINTER_TOKEN_WRONG_DOMAIN;
    }

    pc_pointer_token_lock();
    index = pc_pointer_token_find(token);
    if (index == PC_POINTER_TOKEN_TABLE_SIZE) {
        pc_pointer_token_unlock();
        return PC_POINTER_TOKEN_STALE;
    }
    s_pointer_tokens[index].token = PC_POINTER_TOKEN_TOMBSTONE;
    s_pointer_tokens[index].pointer = 0u;
    s_active_count[domain]--;
    pc_pointer_token_unlock();
    return PC_POINTER_TOKEN_OK;
}

void pc_pointer_token_reset(pc_pointer_token_domain domain) {
    size_t i;

    if (!pc_pointer_token_domain_valid(domain)) {
        return;
    }

    pc_pointer_token_lock();
    for (i = 0; i < PC_POINTER_TOKEN_TABLE_SIZE; i++) {
        uint32_t token = s_pointer_tokens[i].token;

        if (token != 0u && token != PC_POINTER_TOKEN_TOMBSTONE &&
            pc_pointer_token_domain_from_token(token) == domain) {
            s_pointer_tokens[i].token = PC_POINTER_TOKEN_TOMBSTONE;
            s_pointer_tokens[i].pointer = 0u;
        }
    }
    s_active_count[domain] = 0u;
    pc_pointer_token_unlock();
}

uint32_t pc_pointer_token_active_count(pc_pointer_token_domain domain) {
    uint32_t count;

    if (!pc_pointer_token_domain_valid(domain)) {
        return 0u;
    }

    pc_pointer_token_lock();
    count = s_active_count[domain];
    pc_pointer_token_unlock();
    return count;
}
