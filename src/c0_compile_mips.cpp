#include "c0_compile_utils.hpp"
#include "c0_compile_pcode.hpp"
#include "c0_compile_mips.hpp"
#define IN_REG string("$s0")
#define OUT_REG string("$s1")
#define RA_REG string("$ra")
#define V0_REG string("$v0")
#define NUM1 string("$t0")
#define NUM2 string("$t1")
#define NUM3 string("$t2")
#define TMP string("t")
#define GLOBAL string("g")
#define FP string("fp")
#define SP string("sp")
MipsGenerator* handle_mips_generator;

compile_errcode MipsGenerator::FindRelativeAddr(string temp_name, int& addr) {
    auto iter = m_temp2address.find(temp_name);
    if (iter != m_temp2address.end()) {
        addr = iter->second; 
        return COMPILE_OK;
    } else {
        return -3;
    }
}

void MipsGenerator::GenerateStore(string temp_name, string type) {
    if (type == TMP) {
        int store_offset = m_relative_addr;
        m_temp2address[temp_name] = store_offset;
        Output2File(string("sw ") + NUM1 + string(" ") + std::to_string(store_offset) + string("($sp)"));
        m_relative_addr += 4;
    } else if (type == GLOBAL) {
        Output2File(string("sw ") + NUM1 + string(" ") + temp_name + string("($0)"));
    } else {
        Output2File(string("sw ") + NUM1 + string(" ") + temp_name + string("($0)"));
    }
}

void MipsGenerator::GenerateLoad(string target, string source, string type) {
    if (type == TMP) {
        int load_offset;
        FindRelativeAddr(source, load_offset);
        Output2File(string("lw ") + target + string(" ") + std::to_string(load_offset) + string("($sp)"));
    } else if (type == GLOBAL) {
        Output2File(string("lw ") + target + string(" ") + source + string("($0)"));
    } else {
        Output2File(string("lw ") + target + string(" ") + source + string("($fp)"));
    }
}

void MipsGenerator::ExtractString() {
    auto iter = m_pcode_queue.begin();
    for (; iter != m_pcode_queue.end(); ++iter) {
        if (iter->GetOP() == OUTPUT && iter->GetNum2() == STRING) {
            string content = iter->GetNum1();
            Output2File(content + string(": .asciiz \"") + content + string("\""));
        }
    }
}

void MipsGenerator::TranslateADD(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    string num3 = item.GetNum3();
    size_t f2 = num2.find(TMP);
    size_t f3 = num3.find(TMP);
    if (f2 == string::npos && f3 == string::npos) {
        fprintf(stderr, "not implemented translate ADD type\n");
    } else if (f2 != string::npos && f3 != string::npos) {
        GenerateLoad(NUM2, num2, TMP);
        GenerateLoad(NUM3, num3, TMP);
        Output2File(string("add ") + NUM1 + string(" ") + NUM2 + string(" ") + NUM3);
    } else if (f2 == string::npos) {
        GenerateLoad(NUM2, num3, TMP);
        Output2File(string("addi ") + NUM1 + string(" ") + NUM2 + string(" ") + num2);
    } else {
        GenerateLoad(NUM2, num2, TMP);
        Output2File(string("addi ") + NUM1 + string(" ") + NUM2 + string(" ") + num3);
    }
    GenerateStore(num1, TMP);
}

void MipsGenerator::TranslateSUB(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    string num3 = item.GetNum3();

}

void MipsGenerator::TranslateMUL(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    string num3 = item.GetNum3();
    size_t t2 = num2.find(TMP);
    size_t t3 = num3.find(TMP);
    size_t g2 = num2.find(GLOBAL);
    size_t g3 = num3.find(GLOBAL);
    size_t f2 = num2.find(FP);
    size_t f3 = num3.find(FP);
    if (t2 != string::npos) {
        GenerateLoad(NUM2, num2, TMP);
    } else if (g2 != string::npos) {
        string relative_addr = num2.substr(g2 + 1);
        int num_addr = atoi(relative_addr.c_str());
        num_addr += m_global;
        GenerateLoad(NUM2, std::to_string(num_addr), GLOBAL); 
    } else if (f2 != string::npos) {
        string relative_addr = num2.substr(f2 + 2);
        GenerateLoad(NUM2, relative_addr, FP);
    } else {
        Output2File(string("li ") + NUM2 + string(" ") + num2);
    }
    if (t3 != string::npos) {
        GenerateLoad(NUM3, num3, TMP);
    } else if (g3 != string::npos) {
        string relative_addr = num3.substr(g3 + 1);
        int num_addr = atoi(relative_addr.c_str());
        num_addr += m_global;
        GenerateLoad(NUM3, std::to_string(num_addr), GLOBAL); 
    } else if (f3 != string::npos) {
        string relative_addr = num3.substr(f3 + 2);
        GenerateLoad(NUM3, relative_addr, FP);
    } else {
        Output2File(string("li ") + NUM3 + string(" ") + num3);
    }
    Output2File(string("mul ") + NUM1 + string(" ") + NUM2 + string(" ") + NUM3);
    GenerateStore(num1, TMP);
}

void MipsGenerator::TranslateInput(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    if (num2 == string("int")) {
        Output2File(string("li $v0 5"));
    } else if (num2 == string("char")) {
        Output2File(string("li $v0 12"));
    } else {
        fprintf(stderr, "invalid input type\n");
    }
    Output2File(string("move $v0 " + NUM1));
    GenerateStore(num1, TMP);
}

void MipsGenerator::TranslateOutput(Pcode& item) {
    string output_type = item.GetNum2();
    if (output_type == STRING) {
        string name = item.GetNum1();
        Output2File(string("li $v0 4"));
        Output2File(string("la $a0 ") + name);
        Output2File(string("syscall"));
    } else if (output_type == INT_EXPRESSION) {
        string temp_name = item.GetNum1();
        GenerateLoad(OUT_REG, temp_name, TMP);
        Output2File(string("li $v0 1"));
        Output2File(string("move $a0 ") + OUT_REG);
        Output2File(string("syscall"));
    } else if (output_type == CHAR_EXPRESSION) {
        string temp_name = item.GetNum1();
        GenerateLoad(OUT_REG, temp_name, TMP);
        Output2File(string("li $v0 11"));
        Output2File(string("move $a0 ") + OUT_REG);
        Output2File(string("syscall"));
    } else {
        fprintf(stderr, "undefined output behaviour\n");
    }
}

void MipsGenerator::TranslateCall(Pcode& item) {

}

void MipsGenerator::TranslateASSIGN(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    GenerateLoad(NUM2, num2, TMP);
    if (num1 == string("V0")) {
        Output2File(string("move ") + V0_REG + string(" ") + NUM2);
    } else if (num1.find(FP) != string::npos) {
        Output2File(string("move ") + NUM1 + string(" ") + NUM2);
        GenerateStore(num1.substr(num1.find(FP) + 2), FP);
    } else if (num1.find(GLOBAL) != string::npos) {
        Output2File(string("move ") + NUM1 + string(" ") + NUM2);
        string relative_addr = num1.substr(num1.find(GLOBAL) + 1);
        int num_addr = atoi(relative_addr.c_str());
        num_addr += m_global;
        GenerateStore(std::to_string(num_addr), GLOBAL);
    } else {
        Output2File(string("move ") + NUM1 + string(" ") + NUM2);
        GenerateStore(num1, TMP);
    }
}

void MipsGenerator::TranslateJUMP(Pcode& item) {
    string num1 = item.GetNum1();
    if (num1 == string("RA")) {
        //TODO
        Output2File(string("jr $ra"));
    } else {
        Output2File(string("j ") + num1);
    }
}

void MipsGenerator::TranslateBType(Pcode& item) {
    string num1 = item.GetNum1();
    string num2 = item.GetNum2();
    string num3 = item.GetNum3();
    PcodeType op = item.GetOP();
    GenerateLoad(NUM1, num1, TMP);
    GenerateLoad(NUM2, num2, TMP);
    switch (op) {
        case BNE: Output2File(string("bne ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        case BEQ: Output2File(string("beq ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        case BLE: Output2File(string("ble ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        case BLT: Output2File(string("blt ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        case BGE: Output2File(string("bge ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        case BGT: Output2File(string("bgt ") + NUM1 + string(" ") + NUM2 + string(" ") + num3); break;
        default: {
            fprintf(stderr, "invalid b type\n");         
        }
    }
}

void MipsGenerator::Translate() {
    this->ExtractString();
    auto iter = m_pcode_queue.begin();
    for (; iter != m_pcode_queue.end(); ++iter) {
        switch (iter->GetOP()) {
            case LABEL: Output2File(iter->GetNum1() + string(":")); break;
            case OUTPUT: {
                TranslateOutput(*iter);
                break;
            }
            case INPUT: {
                TranslateInput(*iter);
                break;
            }
            case ADD: {
                TranslateADD(*iter);
                break;
            }
            case MUL: {
                TranslateMUL(*iter);
                break;
            }
            case SUB: {
                TranslateSUB(*iter);
                break;
            }
            case ASSIGN: {
                TranslateASSIGN(*iter);
                break;
            }
            case BEQ:
            case BLT:
            case BLE:
            case BGT:
            case BGE:
            case BNE: {
                TranslateBType(*iter);          
                break;
            }
            case CALL: {
                TranslateCall(*iter);
                break;
            }
            case FUNC_BOTTOM: {
                m_relative_addr = 0;                  
                break;
            }
            case JUMP: {
                TranslateJUMP(*iter);
                break;
            }
            default: {
                         std::cout<<iter->GetOP()<<std::endl;
                fprintf(stderr, "not implemented pcode2mips type\n");         
            }
        }
    }
}