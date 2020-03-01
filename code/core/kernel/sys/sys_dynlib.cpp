
/*
 * PS4Delta : PS4 emulation and research project
 *
 * Copyright 2019-2020 Force67.
 * For information regarding licensing see LICENSE
 * in the root of the source tree.
 */

#include <base.h>
#include <logger/logger.h>

#include "kernel/process.h"
#include "kernel/loader/elf_util.h"

#include "error_table.h"
#include "sys_dynlib.h"

namespace kern {
int PS4ABI sys_dynlib_dlopen(const char*) {
    /*TODO: implement, however note that this function is only
    present in devkits*/
    return SysError::eNOSYS;
}

int PS4ABI sys_dynlib_get_info(uint32_t handle, dynlib_info* dyn_info) {
    if (dyn_info->size != sizeof(*dyn_info)) {
        __debugbreak();
        return SysError::eINVAL;
    }

    auto mod = activeProc()->getPrx(handle);
    if (!mod)
        return SysError::eSRCH;

    __debugbreak();
    std::memset(dyn_info, 0, sizeof(dynlib_info));

    std::strncpy(dyn_info->name, mod->name.c_str(), 256);
    std::memcpy(dyn_info->fingerprint, mod->fingerprint, 20);

    std::memcpy(dyn_info->segs, mod->segments, sizeof(mod->segments));
    dyn_info->seg_count = 2;

    return 0;
}

int PS4ABI sys_dynlib_get_info_ex(uint32_t handle, int32_t ukn /*always 1*/,
                                  dynlib_info_ex* dyn_info) {
    if (dyn_info->size != sizeof(*dyn_info)) {
        __debugbreak();
        return SysError::eINVAL;
    }

    auto mod = activeProc()->getPrx(handle);
    if (!mod)
        return SysError::eSRCH;

    std::memset(dyn_info, 0, sizeof(dynlib_info_ex));

    dyn_info->size = sizeof(dynlib_info_ex);
    dyn_info->handle = static_cast<i32>(mod->handle());

    std::strncpy(dyn_info->name, mod->name.c_str(), 256);
    std::memcpy(dyn_info->segs, mod->segments, sizeof(mod->segments));

    dyn_info->seg_count = 0 /*2*/;
    dyn_info->ref_count = mod.use_count();

    if (mod->tlsAddr) {
        dyn_info->tls_index = mod->tlsSlot;
        dyn_info->tls_align = mod->tlsAlign;
        dyn_info->tls_init_size = mod->tlsfSize;
        dyn_info->tls_size = mod->tlsSize;
        dyn_info->tls_init_addr = mod->tlsAddr;
    }

    dyn_info->init_proc_addr = mod->initAddr;
    dyn_info->fini_proc_addr = mod->finiAddr;

    dyn_info->eh_frame_addr = mod->ehFrameAddr;
    dyn_info->eh_frame_size = mod->ehFrameSize;
    dyn_info->eh_frame_hdr_addr = mod->ehFrameheaderAddr;
    dyn_info->eh_frame_hdr_size = mod->ehFrameheaderSize;
    return 0;
}

int PS4ABI sys_dynlib_dlsym(uint32_t handle, const char* symName, void** sym) {
    auto prx = activeProc()->getPrx(handle);
    if (!prx) return -1;

    LOG_INFO("{},{}", prx->name, symName);

    char nameenc[12]{};
    encode_nid(symName, reinterpret_cast<uint8_t*>(&nameenc));

    auto longName = std::string(nameenc) + "#" + prx->name + "#" + prx->name;

    if (auto addrOut = prx->getSymbol(longName.c_str())) {
        *sym = reinterpret_cast<void*>(addrOut);

        return 0;
    }
    LOG_WARNING("unable to find export {} ({})", symName, handle);
    *sym = nullptr;
    return -1;
}

int PS4ABI sys_dynlib_get_obj_member(uint32_t handle, uint8_t index, void** value) {
    if (index != 1) return SysError::eINVAL;

    auto mod = activeProc()->getPrx(handle);
    if (!mod) return -1;


    *value = static_cast<void*>(mod->initAddr);
    return 0;
}

int PS4ABI sys_dynlib_get_proc_param(void** data, size_t* size) {
    auto& mod = activeProc()->main_exec();
    if (mod.param) {
        *data = reinterpret_cast<void*>(mod.param);
        *size = mod.param->size;
        return 0;
    }
    __debugbreak();
    *data = nullptr;
    *size = 0;
    return -1;
}

int PS4ABI sys_dynlib_get_list(uint32_t* handles, size_t maxCount, size_t* count) {
    auto& list = activeProc()->prx_list();

    int listCount = 0;
    for (auto& mod : list) {
        std::printf("MODULE %s -> gets handle %d\n", mod->name.c_str(), mod->handle());

        if (listCount++ > maxCount) break;

        *(handles++) = mod->handle();
    }

    *count = listCount;
    return 0;
}

int PS4ABI sys_dynlib_process_needed_and_relocate() {
    // now done in init_modules
    return 0;
}
} // namespace kern