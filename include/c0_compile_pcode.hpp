#ifndef C0_COMPILE_PCODE_H_
#define C0_COMPILE_PCODE_H_
#include <string>
#include <vector>
#include <stack>
#include <iostream>
using std::string;
using std::vector;
using std::stack;
#define FOREACH_FUNC_PCODE(FUNC) \
        FUNC(ADD) \
        FUNC(SUB) \
        FUNC(MUL) \
        FUNC(DIV) \
        FUNC(ASSIGN) \
        FUNC(CALL) \
        FUNC(BEQ) \
        FUNC(BNE) \
        FUNC(BGE) \
        FUNC(BGT) \
        FUNC(BLE) \
        FUNC(BLT) \
        FUNC(LABEL) \
        FUNC(RETURN) \
        FUNC(JUMP) \
        FUNC(INPUT) \
        FUNC(OUTPUT) \
        FUNC(PARA) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
        
typedef enum _PcodeType {
    FOREACH_FUNC_PCODE(GENERATE_ENUM)
}PcodeType;

extern const char* pcode_string[];

class Pcode {
public:
    Pcode(PcodeType op, string num1, string num2, string num3) : m_op(op), m_num1(num1), m_num2(num2), m_num3(num3) {}
    string ToString() {
        return m_num1 + string("\t") + m_num2 + string("\t") + string(pcode_string[m_op]) + string("\t") + m_num3;
    }
private:
    // num1 : num2 op num3
    PcodeType m_op;
    string m_num1;
    string m_num2;
    string m_num3;
};

class PcodeGenerator {
public:
    explicit PcodeGenerator(const char* pcode_file_name) {
        m_fp_pcode = fopen(pcode_file_name, "w");
        m_temp_count = 0;
        m_label_count = 0;
        m_argument_count = 0;
    }
    ~PcodeGenerator() {
        if (!m_fp_pcode)
            fclose(m_fp_pcode);
    }
    void Insert(Pcode& pcode) {
        m_pcode_queue.push_back(pcode);
    }
    int PcodeLen() {
        return m_pcode_queue.size();
    }
    void PrintSinglePcode(Pcode& pcode) {
        fprintf(m_fp_pcode, "%s\n", pcode.ToString().c_str());
    }
    void PrintAllPcode();
    string GetNextTemp() {
        m_temp_count++;
        string str = string("t") + std::to_string(m_temp_count);
        return str;
    }
    string GetNextLabel() {
        m_label_count++;
        string label = string("label") + std::to_string(m_label_count);
        return label;
    }
    void ZeroArgumentCount() {
      m_argument_count = 0;
    }
    string GetNextArgument() {
        m_argument_count++;
        string argument = string("argument") + std::to_string(m_argument_count);
        return argument;
    }
    void CopyPcode(vector<Pcode>& target_queue);
private:
    int m_temp_count;
    int m_label_count;
    int m_argument_count;
    FILE* m_fp_pcode;
    vector<Pcode> m_pcode_queue;
};
#endif // C0_COMPILE_PCODE_H_
