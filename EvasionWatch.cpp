#include "EvasionWatch.h"
#include "Util.h"
#include "TinyTracer.h"
#include "ModuleInfo.h"

bool EvasionWatch::EvasionAddCallbackBefore(IMG Image, const char* fName, uint32_t argNum, EvasionWatchCallBack callback)
{
    const size_t argMax = 5;
    if (argNum > argMax) argNum = argMax;

    RTN funcRtn = find_by_unmangled_name(Image, fName);
    if (RTN_Valid(funcRtn)) {
        RTN_Open(funcRtn);

        RTN_InsertCall(funcRtn, IPOINT_BEFORE, AFUNPTR(callback),
            IARG_RETURN_IP,
            IARG_ADDRINT, fName,
            IARG_UINT32, argNum,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
            IARG_END
        );

        RTN_Close(funcRtn);
        return true;
    }

    return false;
}

//

EvasionFuncInfo* EvasionWatch::fetchFunctionInfo(const std::string& dllName, const std::string& funcName, t_watch_level maxLevel)
{
    for (size_t i = 0; i < watchedFuncs.funcs.size(); i++) {
        if (util::iequals(dllName, watchedFuncs.funcs[i].dllName)) {
            EvasionFuncInfo& wfunc = watchedFuncs.funcs[i];
            if (wfunc.funcName != funcName) {
                continue;
            }
            if (wfunc.type > maxLevel) {
                continue;
            }
            return &wfunc;
        }
    }
    return nullptr;
}

EvasionFuncInfo* EvasionWatch::fetchSyscallFuncInfo(const std::string& funcName, t_watch_level maxLevel)
{
    EvasionFuncInfo* wfunc = fetchFunctionInfo("ntdll", funcName, maxLevel);
    if (!wfunc) {
        wfunc = fetchFunctionInfo("win32u", funcName, maxLevel);
    }
    return wfunc;
}

size_t EvasionWatch::installCallbacksBefore(IMG Image, EvasionWatchCallBack defaultCallback, t_watch_level maxLevel)
{
    if (!isInit) {
        return 0;
    }
    size_t added = 0;
    const std::string dllName = util::getDllName(IMG_Name(Image));
    for (size_t i = 0; i < watchedFuncs.funcs.size(); i++) {
        if (util::iequals(dllName, watchedFuncs.funcs[i].dllName)) {
            EvasionFuncInfo& wfunc = watchedFuncs.funcs[i];
            if (wfunc.type > maxLevel) {
                continue;
            }
            EvasionWatchCallBack* callback = wfunc.callback;
            if (!callback) {
                callback = defaultCallback;
            }
            if (EvasionAddCallbackBefore(Image, wfunc.funcName.c_str(), wfunc.paramCount, callback)) {
                added++;
            }
        }
    }
    return added;
}