#pragma once
#include <utility>
#include <string>
#include <vector>

std::pair<std::string, size_t> fetch_str(const std::string& src, size_t i, const std::string& delimiter = " ") {
    std::string string;
    while (i < src.size()) {
        if (src.compare(i, delimiter.size(), delimiter) == 0)
            return {string, i + delimiter.size()};
        if (src[i] == '\\' && i + 1 < src.size() && src[i + 1] == '\n') {
            string += '\n';
            i += 2;
            continue;
        }
        if (src[i] == '\n')
            break;
        string += src[i++];
    }
    return {string, i + 1};
}

int names = 0;

std::string generate_name(std::string type) {
    names++;
    return type + std::to_string(names);
}

std::string generate_string_data(std::string name, std::string data, int appendnewline=1) {
    if (appendnewline) return name + " db \"" + data + "\", 0dh, 0ah, 0";
    return name + " db \"" + data + "\", 0";
}

std::string generate_condition(std::string var, std::string cond, std::string value, std::string label) {
    std::string output = "";
    output += "push eax\n";
    output += "mov eax, [" + var + "]\n";
    output += "cmp eax, " + value + "\n";
    output += "pop eax\n";
    if (cond == "=") {
        output += "je " + label + "\n";
    } else if (cond == "<") {
        output += "jb " + label + "\n";
    } else if (cond == ">") {
        output += "ja " + label + "\n";
    } else if (cond == "<=") {
        output += "jbe " + label + "\n";
    } else if (cond == ">=") {
        output += "jae " + label + "\n";
    } else if (cond == "!=") {
        output += "jne " + label + "\n";
    }
    return output;
}

std::string gen_memcpy(std::string src, std::string dest, std::string size) {
    return "push ds\npop es\ncld\nmov si, " + src + "\nmov di, " + dest + "\nmov cx, " + size + "\nrep movsb\n";
}

std::string compilepsos(const std::string& src) {
    std::string code = "bits 16\norg 0\n\nstart:\npush cs\npop ds\n";
    std::string data = "inbuffer times 256 db 0\n";
    std::vector<std::string> labels;
    labels.push_back("start");
    size_t i = 0;
    while (i < src.size()) {
        auto [cmd, next] = fetch_str(src, i, " ");
        i = next;
        if (cmd == "print") {
            auto [string, next] = fetch_str(src, i, "$");
            i = next;
            std::string name = generate_name("str");
            code += "mov si, " + name + "\nint 22h\n";
            data += generate_string_data(name, string);
        } else if (cmd == "rprint") {
            auto [string, next] = fetch_str(src, i, "$");
            i = next;
            std::string name = generate_name("str");
            code += "mov si, " + name + "\nint 22h\n";
            data += generate_string_data(name, string);
        } else if (cmd == "label") {
            auto [string, next] = fetch_str(src, i);
            i = next;
            labels.push_back(string);
            code += string + ":\n";
        } else if (cmd == "goto") {
            auto [string, next] = fetch_str(src, i);
            i = next;
            code += "jmp " + string + "\n";
        } else if (cmd == "var") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i);
            i = next1;
            data += name + " dd " + value + "\n";
        } else if (cmd == "set") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i);
            i = next1;
            code += "mov dword [" + name + "], " + value + "\n";
        } else if (cmd == "if") {
            auto [variable, next] = fetch_str(src, i);
            i = next;
            auto [condition, next1] = fetch_str(src, i);
            i = next1;
            auto [value, next2] = fetch_str(src, i);
            i = next2;
            auto [gotostr, next3] = fetch_str(src, i);
            i = next3;
            auto [label, next4] = fetch_str(src, i);
            i = next4;
            if (gotostr != "goto") throw "no";
            code += generate_condition(variable, condition, value, label);
        } else if (cmd == "alu") {
            auto [action, next] = fetch_str(src, i, " ");
            i = next;
            auto [variable, next1] = fetch_str(src, i, " ");
            i = next1;
            auto [value, next2] = fetch_str(src, i);
            i = next2;
            code += "push eax\n";
            code += "mov eax, [" + variable + "]\n";
            code += action + " eax, " + value + "\n";
            code += "mov dword [" + variable + "], eax\n";
            code += "pop eax\n";
        } else if (cmd == "input") {
            auto [variable, next] = fetch_str(src, i, " ");
            i = next;
            code += "mov si, inbuffer\nint 23h\n";
            code += gen_memcpy("inbuffer", variable, "256");
        } else if (cmd == "str") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i, "$");
            i = next1;
            data += name + " db \"" + value + "\", 0\ntimes 256-($-" + name + ") db 0\n";
        } else if (cmd == "printstrvar") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            code += "mov si, " + name + "\nint 22h\n";
        } else if (cmd == "asm") {
            auto [assembly, next] = fetch_str(src, i, "$");
            i = next;
            code += assembly + "\n";
        } else if (cmd == "load") {
            auto [variable, next] = fetch_str(src, i);
            i = next;
            auto [address, next1] = fetch_str(src, i);
            i = next1;
            code += "push eax\nmov eax, [" + address + "]\n";
            code += "mov [" + variable + "], eax\npop eax\n";
        } else if (cmd == "store") {
            auto [address, next] = fetch_str(src, i);
            i = next;
            auto [variable, next1] = fetch_str(src, i);
            i = next1;
            code += "mov eax, [" + variable + "]\n";
            code += "mov [" + address + "], eax\n";
        } else if (cmd == "copy") {
            auto [destination, next] = fetch_str(src, i);
            i = next;
            auto [source, next1] = fetch_str(src, i);
            i = next1;
            auto [size, next2] = fetch_str(src, i);
            i = next2;
            code += "push ds\n";
            code += "pop es\n";
            code += "cld\n";
            code += "mov si, " + source + "\n";
            code += "mov di, " + destination + "\n";
            code += "mov cx, " + size + "\n";
            code += "rep movsb\n";
        } else if (cmd == "exit") {
            code += "int 21h\n";
        } else if (cmd == "call") {
            auto [label, next] = fetch_str(src, i);
            i = next;
            code += "call " + label + "\n";
        } else if (cmd == "return") {
            auto [number, next] = fetch_str(src, i);
            i = next;
            code += "mov eax, " + number + "\nret\n";
        }
    }
    return code + data;
}

std::string compilep86(const std::string& src) {
    std::string code = "bits 16";
    std::string data = "";
    std::vector<std::string> labels;
    size_t i = 0;
    while (i < src.size()) {
        auto [cmd, next] = fetch_str(src, i, " ");
        i = next;
        if (cmd == "print") {
            auto [string, next] = fetch_str(src, i, "$");
            i = next;
            std::string name = generate_name("str");
            std::string loop = generate_name("print_loop");
            std::string done = generate_name("print_done");
            code += "mov si, " + name + "\n";
            code += loop + ":\n";
            code += "lodsb\n";
            code += "test al, al\n";
            code += "jz " + done + "\n";
            code += "mov ah, 0Eh\n";
            code += "int 10h\n";
            code += "jmp " + loop + "\n";
            code += done + ":\n";
            code += "mov ah, 0Eh\nmov al, 0Dh\nint 10h\nmov al, 0Ah\nint 10h\n";
            data += generate_string_data(name, string);
        } else if (cmd == "label") {
            auto [string, next] = fetch_str(src, i);
            i = next;
            labels.push_back(string);
            code += string + ":\n";
        } else if (cmd == "goto") {
            auto [string, next] = fetch_str(src, i);
            i = next;
            code += "jmp " + string + "\n";
        } else if (cmd == "var") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i);
            i = next1;
            data += name + " dd " + value + "\n";
        } else if (cmd == "set") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i);
            i = next1;
            code += "mov dword [" + name + "], " + value + "\n";
        } else if (cmd == "if") {
            auto [variable, next] = fetch_str(src, i);
            i = next;
            auto [condition, next1] = fetch_str(src, i);
            i = next1;
            auto [value, next2] = fetch_str(src, i);
            i = next2;
            auto [gotostr, next3] = fetch_str(src, i);
            i = next3;
            auto [label, next4] = fetch_str(src, i);
            i = next4;
            if (gotostr != "goto") throw "no";
            code += generate_condition(variable, condition, value, label);
        } else if (cmd == "alu") {
            auto [action, next] = fetch_str(src, i, " ");
            i = next;
            auto [variable, next1] = fetch_str(src, i, " ");
            i = next1;
            auto [value, next2] = fetch_str(src, i);
            i = next2;
            code += "push eax\n";
            code += "mov eax, [" + variable + "]\n";
            code += action + " eax, " + value + "\n";
            code += "mov dword [" + variable + "], eax\n";
            code += "pop eax\n";
        } else if (cmd == "str") {
            auto [name, next] = fetch_str(src, i);
            i = next;
            auto [value, next1] = fetch_str(src, i, "$");
            i = next1;
            data += name + " db \"" + value + "\", 0\ntimes 128-($-" + name + ") db 0\n";
        } else if (cmd == "asm") {
            auto [assembly, next] = fetch_str(src, i, "$");
            i = next;
            code += assembly + "\n";
        } else if (cmd == "load") {
            auto [variable, next] = fetch_str(src, i);
            i = next;
            auto [address, next1] = fetch_str(src, i);
            i = next1;
            code += "push eax\nmov eax, [" + address + "]\n";
            code += "mov [" + variable + "], eax\npop eax\n";
        } else if (cmd == "store") {
            auto [address, next] = fetch_str(src, i);
            i = next;
            auto [variable, next1] = fetch_str(src, i);
            i = next1;
            code += "mov eax, [" + variable + "]\n";
            code += "mov [" + address + "], eax\n";
        } else if (cmd == "copy") {
            auto [destination, next] = fetch_str(src, i);
            i = next;
            auto [source, next1] = fetch_str(src, i);
            i = next1;
            auto [size, next2] = fetch_str(src, i);
            i = next2;
            code += "push ds\n";
            code += "pop es\n";
            code += "cld\n";
            code += "mov si, " + source + "\n";
            code += "mov di, " + destination + "\n";
            code += "mov cx, " + size + "\n";
            code += "rep movsb\n";
        } else if (cmd == "halt") {
            code += "hlt\n";
        } else if (cmd == "call") {
            auto [label, next] = fetch_str(src, i);
            i = next;
            code += "call " + label + "\n";
        } else if (cmd == "return") {
            auto [number, next] = fetch_str(src, i);
            i = next;
            code += "mov eax, " + number + "\nret\n";
        }
    }
    return code + data;
}
