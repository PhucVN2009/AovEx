//https://github.com/geokar2006

void InitIl2cppMethods();

typedef std::vector<const Il2CppAssembly*> AssemblyVector;
typedef unsigned long DWORD;
AssemblyVector *(*Assembly$$GetAllAssemblies_t)();

// Lấy tất cả các Assembly
AssemblyVector *Assembly$$GetAllAssemblies(){
    if (!Assembly$$GetAllAssemblies_t)
        InitIl2cppMethods();
    return Assembly$$GetAllAssemblies_t();
}

// Đảo ngược một chuỗi hex
std::string revhexstr(std::string hex) {
    std::string out;
    for (unsigned int i = 0; i < hex.length(); i += 2) out = hex.substr(i, 2) + out;
    return out;
}

// Sửa định dạng chuỗi hex, bỏ bớt các ký tự không cần thiết
std::string fixhex(std::string str) {
    std::string out = str;
    std::string::size_type tmp;
    if (out.find(std::string("0x")) != -1) {
        tmp = out.find(std::string("0x"));
        out.replace(tmp, 2, std::string(""));
    }
    out.erase(std::remove(out.begin(), out.end(), ' '), out.end());
    return out;
}

// Chuyển đổi chuỗi hex sang DWORD
DWORD HexStr2DWORD(std::string hex) {
    return strtoull(hex.c_str(), NULL, 16);
}

// Kiểm tra xem một chuỗi hex có phải là lệnh B hoặc BL không trên kiến trúc ARM64
bool Is_B_BL_Hex_arm64(std::string hex) {
    DWORD hexw = HexStr2DWORD(revhexstr(fixhex(hex)));
    return (hexw & 0xFC000000) == 0x14000000 || (hexw & 0xFC000000) == 0x94000000;
}

// Kiểm tra xem một chuỗi hex có phải là lệnh B hoặc BL không
bool Is_B_BL_Hex(std::string hex) {
    DWORD hexw = HexStr2DWORD(revhexstr(fixhex(hex)));
    return ((hexw & 0xFC000000) == 0x14000000 || (hexw & 0xFC000000) == 0x94000000) || (hexw & 0x0A000000) == 0x0A000000;
}

// Đọc một chuỗi hex từ bộ nhớ
std::string readHexStrFromMem(const void *addr, size_t len) {
    char temp[len];
    memset(temp, 0, len);
    const size_t bufferLen = len * 2 + 1;
    char buffer[bufferLen];
    memset(buffer, 0, bufferLen);
    std::string ret;
    if (memcpy(temp, addr, len) == NULL)
        return ret;
    for (int i = 0; i < len; i++) {
        sprintf(&buffer[i * 2], "%02X", (unsigned char) temp[i]);
    }
    ret += buffer;
    return ret;
}

// Kiểm tra xem một chuỗi hex có phải là lệnh call không trên kiến trúc x86
bool Is_x86_call_hex(std::string hex) {
    return hex.substr(0, 2) == "E8";
}
/*
 * Branch decoding based on
 * https://github.com/aquynh/capstone/
*/
bool Decode_Branch_or_Call_Hex(const std::string& hex, const DWORD offset, DWORD& outOffset) {
    bool arm = false;
    if (!Is_x86_call_hex(hex)) {
        if (!Is_B_BL_Hex(hex)) {
            return false;
        }
        arm = true;
    }

    const DWORD insn = HexStr2DWORD(revhexstr(fixhex(hex)));
    if (arm) {
        const DWORD imm = ((insn & (((uint32_t)1 << 24) - 1) << 0) >> 0) << 2;
        const DWORD SignExtendedImm = (int32_t)(imm << (32 - 26)) >> (32 - 26);
        outOffset = SignExtendedImm + offset + (Is_B_BL_Hex_arm64(hex) ? 0 : 8);
    } else {
        outOffset = offset + HexStr2DWORD(revhexstr(fixhex(hex)).substr(0, 8)) + 5;
    }

    return true;
}

DWORD FindNext_B_BL_offset(DWORD start, int index = 1) {
#if defined(__arm__) || defined(__aarch64__)
    int offset = 0;
    std::string curHex = readHexStrFromMem((const void*)start, 4);
    DWORD outOffset = 0;
    bool out = true;
    while (!(out = Decode_Branch_or_Call_Hex(curHex, start + offset, outOffset)) || index != 1) {
        offset += 4;
        curHex = readHexStrFromMem((const void*)(start + offset), 4);
        if (out)
            index--;
    }
    return outOffset;
#elif defined(__i386__)
    std::string curHex;
    DWORD outOffset = 0;

    for (int offset = 0; ; ++offset) {
        curHex = readHexStrFromMem(reinterpret_cast<const void*>(start + offset), 1);
        if (Is_x86_call_hex(curHex)) {
            Decode_Branch_or_Call_Hex(readHexStrFromMem(reinterpret_cast<const void*>(start + offset), 5), start + offset, outOffset);
            if (--index == 0) {
                return outOffset;
            }
        }
    }
#endif
}

void InitIl2cppMethods() {
#if defined(__arm__) || defined(__aarch64__)
    const int count = 1;
#elif defined(__i386__) || defined(__x86_64__)
    const int count = 2;
#endif

    if (!Assembly$$GetAllAssemblies_t) {
        auto corlib = il2cpp_get_corlib();
        auto assemblyClass = il2cpp_class_from_name(corlib, "System", "AppDomain");
        const MethodInfo *getAssemb = il2cpp_class_get_method_from_name(assemblyClass, "GetAssemblies", 1);        if (getAssemb) {
            const DWORD GetTypesAdr = FindNext_B_BL_offset(reinterpret_cast<DWORD>(getAssemb->methodPointer), count);
            Assembly$$GetAllAssemblies_t = reinterpret_cast<AssemblyVector *(*)()>(FindNext_B_BL_offset(GetTypesAdr, count + 1));
        } else {
            const DWORD adr = reinterpret_cast<DWORD>(il2cpp_domain_get_assemblies);
            Assembly$$GetAllAssemblies_t = reinterpret_cast<AssemblyVector *(*)()>(FindNext_B_BL_offset(adr, count));
        }
    }
}
