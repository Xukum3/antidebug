#include "Config.h"
#include "Misc.h"
#include "Assembly.h"
#include "Exceptions.h"
#include "Flags.h"
#include "Memory.h"
#include "Timing.h"
#include "ObjectHandles.h"

namespace config {
    // INICONF
    IniConf::IniConf(const std::string& filename) {
        std::ifstream in(filename);
        if (!in) {
            std::cout << "no such file " << filename << "\n";
            m_valid = false;
            return;
        }
        m_name = filename;
        std::string cur_section_name = "GLOBAL";
        std::string buffer;
        std::string accum;

        while (std::getline(in, buffer)) {
            if (buffer.empty()) {
                continue;
            }
            if (buffer[buffer.size() - 1] == '\\') {
                accum += buffer;
                accum.pop_back();
                continue;
            }
            else {
                buffer = accum + buffer;
                accum = "";
            }
            auto str = splitAndClear(buffer, ';');
            if (str.first.empty()) {
                continue;
            }
            if (isSection(str.first)) {
                cur_section_name = getSection(str.first);
                continue;
            }
            m_data[cur_section_name].insert(splitAndClear(str.first, '='));
        }

        m_valid = true;
    }
    IniConf::operator bool() const { return m_valid; }

    std::string IniConf::getName() const noexcept { return m_name; }

    const IniConf::Section&
        IniConf::operator()(const std::string& name) const {
        auto it = m_data.find(name);
        if (it == m_data.end()) {
            return m_empty;
        }
        return it->second;
    }
    bool IniConf::has(const std::string& name) const {
        return (m_data.find(name) != m_data.end());
    }

    bool IniConf::isSection(const std::string& str) const {
        if (str.size() < 3) return false;
        return (str[0] == '[' && str[str.size() - 1] == ']');
    }
    std::string IniConf::getSection(const std::string& str) const {
        if (str.size() < 3) return {};
        return str.substr(1, str.size() - 2);
    }
    std::vector<std::string> IniConf::sections() const {
        std::vector<std::string> ans;
        for (auto&& pair : m_data) {
            if (pair.first != "global") {
                ans.push_back(pair.first);
            }
        }
        return ans;
    }

    // VARIABLE
    IniConf::Variable::Variable(const std::string& data,
        Variable::TYPE type)
        : m_data(data), m_type(type) {}
    IniConf::Variable::operator std::string() const { return m_data; }

    // SECTION
    const IniConf::Variable&
        IniConf::Section::operator()(const std::string& name) const {
        auto it = m_data.find(name);
        if (it == m_data.end()) {
            return m_empty;
        }
        return it->second;
    }
    bool IniConf::Section::has(const std::string& name) const {
        return (m_data.find(name) != m_data.end());
    }

    std::unordered_map<std::string, IniConf::Variable>::const_iterator
        IniConf::Section::begin() const {
        return m_data.begin();
    }
    std::unordered_map<std::string, IniConf::Variable>::const_iterator
        IniConf::Section::end() const {
        return m_data.end();
    }

    void IniConf::Section::insert(
        const std::pair<std::string, std::string>& data) {
        m_data[data.first] = create(data.second);
    }
    IniConf::Variable IniConf::Section::create(const std::string& data) {
        if (data.find(',') != std::string::npos) {
            return Variable(data, Variable::TYPE::ARRAY);
        }
        return Variable(data, Variable::TYPE::VALUE);
    }
    int hash = 0;
    int threads = 0;
    std::unordered_map<std::string, std::string> launch = std::unordered_map<std::string, std::string>();
    std::unordered_map<std::string, void*> functions = std::unordered_map<std::string, void*>( {
        { "Testselfdebug", (void*)TestSelfdebug },
        { "TestCreateToolhelp32SnapshotTls", (void*)TestCreateToolhelp32Snapshot },
        { "Patch_DbgUiRemoteBreakinTls", (void*)Patch_DbgUiRemoteBreakin },
        { "Patch_DbgBreakPoinTlst", (void*)Patch_DbgBreakPoint },
        { "TestSwitch", (void*)TestSwitch },
        { "HideThreadTls", (void*)HideThread },
        { "TestBlockInput", (void*)TestBlockInput },
        { "SuspendDebuggerThreadTls", (void*)SuspendDebuggerThread },

        { "TestInt3", (void*)TestInt3 },
        { "TestInt2D", (void*)TestInt2D },
        { "TestDebugBreak", (void*)TestDebugBreak },
        { "TestICE", (void*)TestICE },
        { "TestTrapFlag", (void*)TestTrapFlag },
        { "TestPopfTrap", (void*)TestPopfTrap },
        { "TestInstrPref", (void*)TestInstrPref },
        { "TestInstructionCounting", (void*)TestInstructionCounting },
        { "TestSelector", (void*)TestSelector },

        { "TestUnhExF", (void*)TestUnhExF },
        { "TestRaiseEx", (void*)TestRaiseEx },
        { "TestControlFlow", (void*)TestControlFlow },
        { "TestCtrlEvent", (void*)TestCtrlEvent },

        { "TestIsDebuggerPresentTls", (void*)TestIsDebuggerPresent },
        { "TestCheckRemoteDebuggerPresentTls", (void*)TestCheckRemoteDebuggerPresent },
        { "TestProcessDebugPortTls", (void*)TestProcessDebugPort },
        { "TestProcessDebugFlagsTls", (void*)TestProcessDebugFlags },
        { "TestSystemKernelDebuggerInformationTls", (void*)TestSystemKernelDebuggerInformation },
        { "TestRtlQueryTls", (void*)TestRtlQuery },
        { "TestNtGlobalFlagTls", (void*)TestNtGlobalFlag },
        { "TestHeapCheckTls", (void*)TestHeapCheck },

        { "TestSpecByte", (void*)TestSpecByte },
        { "TestMemoryBreak", (void*)TestMemoryBreak },
        { "TestHardBreak", (void*)TestHardBreak },
        { "TestFuncPatch", (void*)TestFuncPatch },
        { "TestHashsum", (void*)TestHashsum },

        { "TestFindWindowTls", (void*)TestFindWindow },
        { "TestCreateFileTls", (void*)TestCreateFile },
        { "TestCloseHandleTls", (void*)TestCloseHandle },
        { "TestLoadLibraryTls", (void*)TestLoadLibrary },
        { "TestCsrGetProcessIdTls", (void*)TestCsrGetProcessId },
        { "TestQueryObjectTls", (void*)TestQueryObject },

        { "base_measure_time", (void*)base_measure_time },
        { "timer_timeGetTime", (void*)timer_timeGetTime },
        { "timer_timegetsystime", (void*)timer_timegetsystime },
        { "timer_timegetsystimeasfile", (void*)timer_timegetsystimeasfile },
        { "timer_QueryPerfomanceCounter", (void*)timer_QueryPerfomanceCounter },
        { "timer_gettickcount", (void*)timer_gettickcount },
        { "timer_gettick64count", (void*)timer_gettick64count },
        { "timer_getlocaltime", (void*)timer_getlocaltime },
        { "timer_rdtsc", (void*)timer_rdtsc },
        { "timer_partstiming", (void*)timer_partstiming },
    } );

    void readconfig(IniConf& conf) {
        launch["Testselfdebug"] = { conf("MISC")("Testselfdebug") };
        launch["TestCreateToolhelp32SnapshotTls"] = { conf("MISC")("TestCreateToolhelp32Snapshot") };
        launch["Patch_DbgUiRemoteBreakinTls"] = { conf("MISC")("Patch_DbgUiRemoteBreakin") };
        launch["Patch_DbgBreakPointTls"] = { conf("MISC")("Patch_DbgBreakPoint") };
        launch["TestSwitch"] = { conf("MISC")("TestSwitch") };
        launch["HideThreadTls"] = { conf("MISC")("HideThread") };
        launch["TestBlockInput"] = { conf("MISC")("TestBlockInput") };
        launch["SuspendDebuggerThreadTls"] = { conf("MISC")("SuspendDebuggerThread") };

        launch["TestInt3"] = { conf("ASSEMBLY")("TestInt3") };
        launch["TestInt2D"] = { conf("ASSEMBLY")("TestInt2D") };
        launch["TestDebugBreak"] = { conf("ASSEMBLY")("TestDebugBreak") };
        launch["TestICE"] = { conf("ASSEMBLY")("TestICE") };
        launch["TestTrapFlag"] = { conf("ASSEMBLY")("TestTrapFlag") };
        launch["TestPopfTrap"] = { conf("ASSEMBLY")("TestPopfTrap") };
        launch["TestInstrPref"] = { conf("ASSEMBLY")("TestInstrPref") };
        launch["TestInstructionCounting"] = { conf("ASSEMBLY")("TestInstructionCounting") };
        launch["TestSelector"] = { conf("ASSEMBLY")("TestSelector") };

        launch["TestUnhExF"] = { conf("EXCEPTIONS")("TestUnhExF") };
        launch["TestRaiseEx"] = { conf("EXCEPTIONS")("TestRaiseEx") };
        launch["TestControlFlow"] = { conf("EXCEPTIONS")("TestControlFlow") };
        launch["TestCtrlEvent"] = { conf("EXCEPTIONS")("TestCtrlEvent") };

        launch["TestIsDebuggerPresentTls"] = { conf("FLAGS")("TestIsDebuggerPresent") };
        launch["TestCheckRemoteDebuggerPresentTls"] = { conf("FLAGS")("TestCheckRemoteDebuggerPresent") };
        launch["TestProcessDebugPortTls"] = { conf("FLAGS")("TestProcessDebugPort") };
        launch["TestProcessDebugFlagsTls"] = { conf("FLAGS")("TestProcessDebugFlags") };
        launch["TestSystemKernelDebuggerInformationTls"] = { conf("FLAGS")("TestSystemKernelDebuggerInformation") };
        launch["TestRtlQueryTls"] = { conf("FLAGS")("TestRtlQuery") };
        launch["TestNtGlobalFlagTls"] = { conf("FLAGS")("TestNtGlobalFlag") };
        launch["TestHeapCheckTls"] = { conf("FLAGS")("TestHeapCheck") };

        launch["TestSpecByte"] = { conf("MEMORY")("TestSpecByte") };
        launch["TestMemoryBreak"] = { conf("MEMORY")("TestMemoryBreak") };
        launch["TestHardBreak"] = { conf("MEMORY")("TestHardBreak") };
        launch["TestFuncPatch"] = { conf("MEMORY")("TestFuncPatch") };
        launch["TestHashsum"] = { conf("MEMORY")("TestHashsum") };

        launch["TestFindWindowTls"] = { conf("DESCRIPTORS")("TestFindWindow") };
        launch["TestCreateFileTls"] = { conf("DESCRIPTORS")("TestCreateFile") };
        launch["TestCloseHandleTls"] = { conf("DESCRIPTORS")("TestCloseHandle") };
        launch["TestLoadLibraryTls"] = { conf("DESCRIPTORS")("TestLoadLibrary") };
        launch["TestCsrGetProcessIdTls"] = { conf("DESCRIPTORS")("TestCsrGetProcessId") };
        launch["TestQueryObjectTls"] = { conf("DESCRIPTORS")("TestQueryObject") };

        launch["base_measure_time"] = { conf("TIMING")("base_measure_time") };
        launch["timer_timeGetTime"] = { conf("TIMING")("timer_timeGetTime") };
        launch["timer_timegetsystime"] = { conf("TIMING")("timer_timegetsystime") };
        launch["timer_timegetsystimeasfile"] = { conf("TIMING")("timer_timegetsystimeasfile") };
        launch["timer_QueryPerfomanceCounter"] = { conf("TIMING")("timer_QueryPerfomanceCounter") };
        launch["timer_gettickcount"] = { conf("TIMING")("timer_gettickcount") };
        launch["timer_gettick64count"] = { conf("TIMING")("timer_gettick64count") };
        launch["timer_getlocaltime"] = { conf("TIMING")("timer_getlocaltime") };
        launch["timer_rdtsc"] = { conf("TIMING")("timer_rdtsc") };
        launch["timer_partstiming"] = { conf("TIMING")("timer_partstiming") };
   
        hash = { conf("AUX")("hash").as<int>() };
        threads = { conf("AUX")("threads").as<int>() };
    }
} // namespace config
