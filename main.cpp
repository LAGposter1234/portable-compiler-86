#include "pcompiler.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <algorithm>
#define VERSION "0.1" // NOTICE NOTICE NOTE UPDATE THIS NUMBER

struct ForLoop {
    std::string start, end, var, action, step;
};

std::string compileppp(const std::string& src) {
    int while_id = 0;
    std::vector<std::pair<std::string, std::string>> while_stack;
    std::vector<ForLoop> for_stack;
    int for_id = 0;
    std::string out;
    std::vector<std::string> vars;
    std::istringstream input(src);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string a, b, c;
        ss >> a;

        if (a == "let") {
            ss >> b >> c;
            if (b.empty() || c != "=") continue;
            std::string value;
            ss >> value;
            vars.push_back(b);
            out += "var " + b + " " + value + "\n";
        } else if (a == "goto") {
            ss >> b;
            out += "goto " + b + "\n";
        } else if (a == "print" || a == "rprint" || a == "input" || a == "if" ||
a == "var" || a == "set" || a == "alu" || a == "label" || a == "str" || a
== "asm" || a == "load" || a == "store" || a == "copy" || a == "exit" || a == "call") {
            out += line + "\n";
        } else if (a == "while") {
            std::istringstream ss(line);
            std::string keyword, lhs, op, rhs;
            ss >> keyword >> lhs >> op >> rhs;
            int id = while_id++;
            std::string start = "while_" + std::to_string(id);
            std::string end = "endwhile_" + std::to_string(id);
            std::string inverse;
            if (op == "<") inverse = ">=";
            else if (op == "<=") inverse = ">";
            else if (op == ">") inverse = "<=";
            else if (op == ">=") inverse = "<";
            else if (op == "==") inverse = "!=";
            else if (op == "!=") inverse = "==";
            else throw std::runtime_error("invalid while comparison");
            out += "label " + start + "\n";
            out += "if " + lhs + " " + inverse + " " + rhs + " goto " + end + "\n";
            while_stack.push_back({start, end});
        } else if (a == "endw") {
            if (while_stack.empty()) throw std::runtime_error("unexpected end");
            out += "goto " + while_stack.back().first + "\n";
            out += "label " + while_stack.back().second + "\n";
            while_stack.pop_back();
        } else if (a == "for") {
            std::istringstream ss(line);
            std::string keyword, var, equals, initial, lhs, op, rhs, action, step;
            ss >> keyword >> var >> equals >> initial >> lhs >> op >> rhs >> action >> step;

            if (var.empty() || equals != "=" || lhs != var || step.empty())
                throw std::runtime_error("invalid for syntax");

            int id = for_id++;
            std::string start = "for_" + std::to_string(id);
            std::string end = "endfor_" + std::to_string(id);
            std::string inverse;

            if (op == "<") inverse = ">=";
            else if (op == "<=") inverse = ">";
            else if (op == ">") inverse = "<=";
            else if (op == ">=") inverse = "<";
            else if (op == "==") inverse = "!=";
            else if (op == "!=") inverse = "==";
            else throw std::runtime_error("invalid for comparison");

            if (action != "inc" && action != "dec")
                throw std::runtime_error("expected inc or dec");

            out += "var " + var + " " + initial + "\n";
            out += "label " + start + "\n";
            out += "if " + lhs + " " + inverse + " " + rhs + " goto " + end + "\n";

            for_stack.push_back({start, end, var, action, step});
        } else if (a == "endf") {
            if (for_stack.empty())
                throw std::runtime_error("unexpected endf");

            auto loop = for_stack.back();
            for_stack.pop_back();

            out += "alu " + std::string(loop.action == "inc" ? "add " : "sub ")
            + loop.var + " " + loop.step + "\n";
            out += "goto " + loop.start + "\n";
            out += "label " + loop.end + "\n";
        } else {
            std::string op;
            ss >> op;

            if (op == "=") {
                std::string value;
                ss >> value;
                out += "set " + a + " " + value + "\n";
            } else if (op == "+=") {
                std::string value;
                ss >> value;
                out += "alu add " + a + " " + value + "\n";
            } else if (op == "-=") {
                std::string value;
                ss >> value;
                out += "alu sub " + a + " " + value + "\n";
            } else {
                if (line.back() == ':')
                    out += "label " + line.substr(0, line.size() - 1) + "\n";
                else
                    out += line + "\n";
            }
        }
    }

    return out;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    if (argv[1] == "-v") {
        std::cout << "Portable Compiler version " VERSION "\n";
        return 0;
    }
    if (argc < 4) return 1;


    std::ifstream file(argv[2]);
    if (!file)
        return 1;

    std::string src((std::istreambuf_iterator<char>(file)), {});

    std::string mode = argv[1];
    target = "-86";
    if (argc > 4) {
        target = argv[4];
    }

    if (mode == "-p") {
        if (target == "-sos") {
            std::string asmout = compilepsos(src);
        } else if (target == "-86") {
            std::string asmout = compilep86(src);
        } else {
            throw "no";
        }
        {
            std::ofstream tmpfile("/tmp/output.asm");
            if (!tmpfile)
                return 1;
            tmpfile << asmout;
        }

        execlp("nasm", "nasm", "-f", "bin", "/tmp/output.asm", "-o", argv[3], (char*)nullptr);
        return 1;
    } else if (mode == "-p++") {
        std::string psrc = compileppp(src);
        if (target == "-sos") {
            std::string asmout = compilepsos(src);
        } else if (target == "-86") {
            std::string asmout = compilep86(src);
        } else {
            throw "no";
        }

        {
            std::ofstream tmpfile("/tmp/output.asm");
            if (!tmpfile)
                return 1;
            tmpfile << asmout;
        }

        execlp("nasm", "nasm", "-f", "bin", "/tmp/output.asm", "-o", argv[3], (char*)nullptr);
        return 1;
    } else {
        throw "no";
    }
    return 0;
}
