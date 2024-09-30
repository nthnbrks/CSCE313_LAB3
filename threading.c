#include <threading.h>

void t_init()
{
        for (int i = 0; i < NUM_CTX; ++i){
                contexts[i].state = INVALID;
        }
        current_context_idx = NUM_CTX;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // find an empty slot
        volatile int idx = -1;
        for (volatile int i = 0; i < NUM_CTX; ++i){
                if (contexts[i].state == INVALID){
                        idx = i;
                        break;
                }
        }

        if (idx == -1){
                return 1; // no empty slots found
        }

        if (getcontext(&contexts[idx].context) == -1){
                return 1; // context coulndn't be retrieved
        }

        contexts[idx].context.uc_stack.ss_sp = malloc(STK_SZ);
        if (!contexts[idx].context.uc_stack.ss_sp){
                return 1; // stack wasn't allocated
        }
        contexts[idx].context.uc_stack.ss_size = STK_SZ;
        contexts[idx].context.uc_stack.ss_flags = 0;
        contexts[idx].context.uc_link = NULL;

        makecontext(&contexts[idx].context, (ctx_ptr)foo, 2, arg1, arg2);
        contexts[idx].state = VALID;

        return 0;
}

int32_t t_yield()
{
        uint8_t next_context = current_context_idx;
        uint8_t next_valid_context;
        int found_valid_context = 0;
        int valid_count = 0;

        for (uint8_t i = 0; i < NUM_CTX; i++) {
                next_context = (current_context_idx + i) % NUM_CTX;
                if (contexts[next_context].state == VALID) {
                        if (found_valid_context == 0){
                                found_valid_context = 1;
                                next_valid_context = next_context;
                        }
                        valid_count++;
                }
        }

        if (next_valid_context == current_context_idx){
                return 1; // no next valid context to yield to
        }
        int last_current_context_idx = current_context_idx;
        current_context_idx = next_valid_context;
        swapcontext(&contexts[last_current_context_idx].context,&contexts[next_valid_context].context);
        return valid_count;
}

void t_finish()
{
        if (current_context_idx != NUM_CTX) {
                free(contexts[current_context_idx].context.uc_stack.ss_sp);
                contexts[current_context_idx].state = DONE;
                t_yield();
        }
}
