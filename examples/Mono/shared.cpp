#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <inttypes.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

#include <MemoryModifier/Process.h>
#include <MemoryModifier/MonoManager.h>

typedef void (*BreakWall)(void* self);

static uint32_t oStaminaCoolDownTime;
static BreakWall mBreakWall;

// We disable the abilty to get damage, we also get
// the address to the player by doing this, so we
// also replace the stamina delay to 0 -> speed hack
extern "C" void Damage(void* self) {
    memset((void*)((intptr_t)self+oStaminaCoolDownTime), 0, 4);
}

extern "C" void TryBreakWall(void* self, void* stage) {
    mBreakWall(self);
}

void __attribute__ ((constructor)) init() {
    Process self;
    MonoManager mono(self);

    intptr_t cPlayerController = mono.getClass("PlayerController");
    intptr_t mDamage = mono.getMethodOfClass(cPlayerController, "Damage");
    oStaminaCoolDownTime = mono.getFieldOffsetOfClass(cPlayerController, "_staminaCoolDownTime");
    self.detourFunction(mDamage, (intptr_t)Damage);

    intptr_t cBreakableWall = mono.getClass("BreakableWall");
    intptr_t mTryBreakWall = mono.getMethodOfClass(cBreakableWall, "TryBreakWall");
    intptr_t mBreakWall = mono.getMethodOfClass(cBreakableWall, "BreakWall");
    self.detourFunction(mTryBreakWall, mBreakWall);

    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    printf("Unload injected lib\n");
}
